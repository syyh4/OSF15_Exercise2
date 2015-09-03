# OS F15 Block Storage System

Here's what you need to do:
- Fork this repo
- Install the [needed libraries](https://github.com/mizzoucs/OSF15_Library/)
	- Feel free to check out dynamic_array, too! I worked my butt off putting it together!
- Read the header and implementation, figure out what everything does
- Have fun
- Comment the functions in the implementation
- Implement (and comment...) the write, import, and utility functions
- Learn things
- Submit a pull request

The generate_drive application will generate a functioning block storage device for you to test with. Just run the program (with no arguments) to see how to use it. The drive files created will have the following properties
- An empty, full, or random FBM (with the FBM block bits correctly set)
- Blocks made up of 512 uint16_ts containing that block's id number (ex: block 8 is just the number 0x0008 over and over)

We (I) suggest you use .bs as your block storage device's extension. Anything with .bs in the name will be ignored by git as well as any folder called build and a bunch of other things. Check out the .gitignore! It's handy!


-- Will, the best TA
