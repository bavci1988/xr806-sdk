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

Most of the MAC layer lives in the ROM of the WLAN core, which is not in this
repository. Changing firmware behaviour therefore needs a ROM dump first.

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
