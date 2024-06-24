# Tetris PS5

This is a Tetris game developed in C using SDL2, designed to be played on a PS5 Jailbreak (3.xx-4.xx).

## Prerequisites
- elfldr.elf
- shsrv.elf
- ftpsrv.elf

This game is also compatible with websrv.

For more information and access to required payloads, please visit: [PS5 Payload Dev](https://github.com/ps5-payload-dev)

## How to Use
1. Send the elfldr.elf payload do port 9020.
1. Send the shsrv.elf payload to port 9021.
2. Send the ftpsrv.elf payload to port 9021.
3. Copy the tetris.elf payload and tetris.wav/demo.ogg to `/data` directory on your PS5.
4. Copy the arial.ttf font to `/data` directory on your PS5.
4. Execute `telnet [PS5-IP] 2323`.
5. Run the command `hbldr /data/tetris.elf -m /data/tetris.wav -f /data/arial.ttf` (or `hbldr /data/tetris.elf --help`) to start the game.

## Credits
Credits to J. Tornblom for the SDK, SDL2 port for PS5, and required payloads.
