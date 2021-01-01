# z64playerRenderInit
This OoT/MM mod allows a function to be embedded within and run from Link's ZOBJs once per frame, enabling us to do fun effects like flowing capes, glowing materials, physics, scrolling textures, different tunics, etc.

## Showcase
 * `TODO` i didnt write this for nothing i want to see people do cool stuff with it and they will be organized here and people can download and use them for their own custom playas mods and it's gonna be great

## Prerequisites
 * `zzplayas` rev 4b or newer
 * `zzconvert` rev 8b or newer
 * `z64ovl` or `Sharp Ocarina`
 * your `zzplayas` manifest must use the `POOL` feature
 * the `b` stands for `beta`; if you aren't a collaborator you will have to wait until the official releases ;(

## Building (`z64ovl`)
 * change the `#include`s to be for your specific version or game
 * run `make clean && make`
 * `func.ovl` and `mod.bin` are generated

## Building (`Sharp Ocarina`)
 * this is how most people will use this
 * use `Sharp Ocarina` to build `func.c` (it's super straightforward)
 * `func.ovl` will be generated, which you then use in `zzplayas`
 * actually i meant `Custom Actor Toolkit` my bad

## Warning
 * the assembly at the start of `func.c` is a hacky way of trying to guarantee a jump to the `renderinit` function will be at the beginning of the overlay's `.text` section
 * it is very important that the beginning of the overlay's `.text` section be `renderinit`, or a jump to `renderinit`
 * it is important that `main.c` (the mod) compiles in such a way that the function `main` has the address `0x80800000`

## OoT debug
 * write `mod.bin` at `0xBB36FC`
 * write `00000000 00000000 0C04F157` at `0xB0E688` (this hook is subject to change) (`TODO` use a better one)
 * write `00000000` at `0xFE17FC`
 * write `00000000` at `0x10197FC`
 * `TODO` this should be made an easy-to-use patch when finalized

## OoT NTSC 1.0
 * `TODO` someone please get this working for oot ntsc 1.0 my dog ate my expansion pak and master quest is hard

## Using `func.ovl` once you've built or downloaded one
 * `func.ovl` goes in the `Physics ZOVL` field in `zzplayas`
 * make sure you have patched your game to be compatible with this tweak first
 * wow how easy

## Notes
 * This expects the `uint32_t` at `0x57FC` in both of Link's ZOBJs to point to the end of an overlay embedded into the ZOBJ. The pointer is simply the offset within the ZOBJ: no ram segment required. The value `0` signifies no overlay is present.
 * The two `uint32_t`s at `0x57F0` are the `start` and `end` offsets of the block that should be transferred for OoT Online, respectively.
 * If you have no idea what any of that means, you should be thankful.
 * Compatible with OoT Online™ (ideally, these patches will be embedded into it and it will do all the work for the user)
