This tool can be used to dump or flash Xbox 360 NAND. Can be useful for some hacks like Reset Glitching. 
GPIOD is used, so any computer with GPIO can be used. It was tested with a Raspberry PI. 

Usage:

To compile, install libgpiod and run make.
 
To read 16mb to dump.bin: ./nand360tool -s 16 -o dump.bin
Write dump file back to NAND: ./nand360tool -i dump.bin
Write only the first 3mb: ./nand360tool -i dump.bin -s 3
