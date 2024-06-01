# Tetris PS5

This is a Tetris game developed in C using SDL2, designed to be played on a PS5 Jailbreak (3.xx-4.xx).

## Prerequisites
- hbldr.elf
- shsrv.elf
- ftpsrv.elf

This game is also compatible with websrv.

For more information and access to required payloads, please visit: [PS5 Payload Dev](https://github.com/ps5-payload-dev)

## How to Use
1. Send the hbldr.elf payload to port 9021.
2. Send the shsrv.elf payload to port 9021.
3. Send the ftpsrv.elf payload to port 9021.
4. Copy the tetris.elf payload and tetris.wav to `/data` directory on your PS5.
5. Execute `telnet [PS5-IP] 2323`.
6. Run the command `hbldr /data/tetris.elf` (or `hbldr /data/tetris.elf /data/tetris.wav`) to start the game.

## Credits
Credits to J. Tornblom for the SDK, SDL2 port for PS5, and required payloads.
