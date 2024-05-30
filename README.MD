# Tetris PS5

## Description
This is a Tetris game developed in C using SDL2, designed to be played on a PS5 with Jailbreak firmware (3.xx-4.xx). It requires specific payloads to be sent to the PS5 via specific ports, allowing you to run the game on your console.

## Prerequisites
- hbldr.elf
- shsrv.elf
- ftpsrv.elf

This game is also compatible with websrv.

For more information and access to required payloads, please visit: [PS5 Payload Dev](https://github.com/ps5-payload-dev)

## How to Use
1. Send the hbldr.elf payload to port 9021 of your PS5.
2. Send the shsrv.elf payload to port 9021 of your PS5.
3. Send the ftpsrv.elf payload to port 9021 of your PS5.
4. Copy the tetris.elf payload to `/data` directory on your PS5.
5. Execute `telnet [PS5-IP] 2323`.
6. Run the command `hbldr /data/tetris.elf` (or `hbldr /data/tetris.elf /data/audio.wav`) to start the game.

## Credits
Credits to J. Tornblom for the SDK, SDL2 port for PS5, and other payloads.
