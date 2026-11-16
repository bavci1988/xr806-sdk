# XR806 WLAN firmware notes

Findings from reverse engineering `bin/xradio_v3/wlan_fw.bin` and
`lib/xradio_v3/libxrwireless.a` (the objects keep full DWARF debug info).
Nothing here is verified on hardware unless it says so.

## Wake sources

- ARP requests do not wake the CPU (found earlier by reverse engineering).
- Home Assistant's API keepalive does: aioesphomeapi sends a `PingRequest`
  when it has received no valid message from the device for 20 s
  (`KEEP_ALIVE_FREQUENCY`). Any message from the device resets that timer, and
  an unsolicited `PingResponse` has no handler, so Home Assistant drops it.

## `wlan_fw.bin` layout

The file is a patch for the ROM of the WLAN core, not a full firmware.

- Header: magic `XR01`, then records of `{flags, load address, length}` and
  the data. A record with `flags = 1` and length 0 ends a group.
- First group (ARM code and data at `0xfff00000`): reset code that sets up
  the stacks of the ARM modes, then jumps into ROM (`0x000054e7`, Thumb), and
  at `0xfff00200` a table of function pointers into ROM (`0x0000xxxx`,
  `0x0001xxxx`) and into the patch (`0xfff0xxxx`, `0x0900xxxx`). The patch
  replaces single entries of that table.
- Second group (Thumb code at `0x09000400` and `0x09009c00`, about 13 KB),
  followed by register tables at `0x0ab8xxxx` and more code at `0xfff01098`.

Most of the MAC layer lives in the ROM of the WLAN core. The ROM is not in the
M33's address space (only the 96 KB WLAN SRAM is, at 0x68000000). It was read
through the stock firmware with the register debug request (MIB 9) and is kept in
the private Porting-lab repository (`WXU_XR806_wlan_core_rom_128KiB.bin`,
CRC-32 `6F0973D7`).

Full record format: `{0, address, length, data}` loads, `{1, entry, 0}` ends a
group and is followed by a 4-byte group header, `{2, length, data}` is a
register write list. Load addresses: 0xFFF00000/0xFFF00200/0xFFF01098/
0xFFF088B0/0xFFF0A1F8/0xFFF0A22C, 0x09000400/0x09001884/0x09009C00/0x0900BB84,
register tables at 0x0AB81000 and 0x0AB88400..0x0AB88C00. The host driver maps
WLAN 0xFFF0xxxx to 0x6800xxxx and WLAN 0x0900xxxx to 0x6800C000 + offset
(`FW_ADDR_TO_APP_ADDR`); 0xFFF0C000 and 0x09000000 are the same RAM.

## TCP keepalive offload ("P2P server")

`wlan_ext_request()` turns the four commands into MIB writes
(`xradio_user_config()` in `libxrwireless.a`):

| Command                           | MIB    | Length |
|-----------------------------------|--------|--------|
| `WLAN_EXT_CMD_SET_P2P_SVR`        | 0x1076 | 92     |
| `WLAN_EXT_CMD_SET_P2P_WKP_CFG`    | 0x1077 | 432    |
| `WLAN_EXT_CMD_SET_P2P_KPALIVE_CFG`| 0x1078 | 400    |
| `WLAN_EXT_CMD_SET_P2P_HOST_SLEEP` | 0x1079 | 4      |

- For `P2P_SVR` the driver overwrites `EncHdrSize`/`EncTailSize` with the
  802.11 cipher overhead (WEP 4/4, TKIP 8/12, CCMP 8/8). The firmware builds
  and encrypts the 802.11 frames itself.
- The vendor example (`project/common/cmd/cmd_wlancmd.c`) sets
  `TcpSeqInit = lwip_getsockack()` and `TcpAckInit = lwip_getsockseq()`, which
  return the PCB's `snd_nxt` and `rcv_nxt`.
- The firmware reports back only a server index and a state
  (`xr_msg_ipc_p2p_ind_proc`, inlined in `xr_msg_rx_process`): 2 = wake packet,
  3 = keepalive lost. It reports no sequence number and no count of sent
  packets.
- The keepalive sends one fixed payload. Encrypted API frames need a new Noise
  nonce for every frame, so a repeated frame cannot carry more than one valid
  message.

In esphome-xr806, `xr806: experimental_wifi_fw_standby_patch:` is the only
switch for changing the WLAN firmware's standby behaviour. Today it puts the API
connection on this keepalive to observe how the firmware behaves; a patched
`wlan_fw.bin` will be selected by the same switch.

## Keepalive implementation (in the patch, not the ROM)

The ROM's MIB write dispatcher (0x14674) first calls a patch hook through a
function pointer (0x0900A226). MIBs 0x1076..0x1079 are handled entirely there:

| Address | Role |
|---|---|
| 0x0900A226 | MIB pre-handler: 0x1076 copies the server table (to 0x0900BB8C, 0x1C per server: +8 Enable, +0xA IPIdInit, +0xC TcpSeqInit, +0x10 TcpAckInit), 0x1077 the wake table, 0x1078 the keepalive table, 0x1079 sets host sleep |
| 0x0900A330 | MIB 0x1078: per enabled server a retransmit timer, then **an immediate send** (`bl 0x0900A6C8` at 0x0900A374), then the period timer (`KeepAlivePeriod_s` x 10^6 us) |
| 0x0900A6C8(i) | send: TCP frame via 0x09009DAC(vif, i, 1), retry count +1, retransmit timer `TxTimeOut_s` |
| 0x09009DAC | builds the IPv4/TCP frame; copies the payload at 0x09009E0C from keepalive entry i (+6 length, +8 payload) |
| 0x0900A720(i, ack) | ACK received: sequence += PayloadLen, IP ID += 1, retransmit timer stopped |
| 0x0900A75C | period timer: sends for every enabled server |
| 0x0900A7A6(i) | retransmit timeout: resend, or after `TxRetryLimit` disable the server and report state 3 (0x0900A5B8) |
| 0x0900A3C6 | UDP keepalive, only called for `TcpOrUdp & 2` |

Run-time state per server at 0x0900BBE8 + 0x1C + i*0x24 (WLAN address; M33
0x68017C04 + i*0x24): +0 sequence, +4 acknowledgement, +8 IP ID, +0xC retries,
+0xD frame in flight. The number of acknowledged keepalive frames is
`IP ID - IPIdInit`.

### Extension (esphome-xr806, experimental_wifi_fw_standby_patch)

Implemented in `esphome/components/xr806/build_support/wlan_fw_patch.py` and
`wlan_fw_keepalive.S`; applied at build time to the stock file (SHA-256
`0d08b6c6…`), nothing patched is stored. The file's checksum is the last word
of record type 4: all 32-bit words of the file sum to zero. Besides the two
changes below, data segments from the peer are passed to the host
(0x0900A14C `beq` -> `bne 0x0900A220`) instead of being acknowledged and
dropped by the firmware.

Original plan:

- Remove the immediate send at 0x0900A374 so the first frame goes out one
  period after configuration, not while the host suspends the WLAN.
- At 0x09009E0C choose frame k = `IP ID - IPIdInit` from a list of equally long
  pre-encrypted frames in the payload areas of all three keepalive entries
  (5 frames of 23 bytes each); code in the UDP keepalive's space.
- The host wakes on its own timer before the list runs out, reads k from the
  state above, moves lwIP's sequence by k frames and sends the frames the
  firmware did not send itself, so its Noise counter matches Home Assistant's.
