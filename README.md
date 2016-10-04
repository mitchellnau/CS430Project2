This program was made by Mitchell Hewitt for CS430 Computer Graphics (Section 1), Project 2 - Basic Raycaster, in Fall 2016
This program reads in a height, width, json file, and output .ppm file.  
It parses the objects in the json file into a visual representation using raycasting.
The visual representation is then written out as an output p3 .ppm image file.

To use this program...

	1.  Compile it with the provided makefile (requires gcc).

	2.  Use the command "200 200 input.json output.ppm" to read the input json file
	    and write the objects within that json file to a p3 output.ppm 200x200 pixel image file.

If you would like to verify the raycasting...

	1.  Open the input file using a text editor that can read .json files.
	    You will find:
		> A camera object.

		> A sphere object with R=0.5, G=0.75, B=0, centered at 1, -1, 10 with a radius 1.
		  This object should be greenish, 10 units away from the camera, one unit to the right horizontally, one unit down vertically, and circular

		> A sphere object with R=1.0, G=0, B=0, centered at 0, -1, 20 with a radius 2.
		  This object should be red, 20 units away from the camera, centered horizontally, one unit down, and circular.
		  Since this object has a greater z-value than the green sphere, it should be behind the green sphere.
		  
		> A sphere object with R=0.5, G=0, B=0.5, centered at 0, 3, 20 with a radius 1.
		  This object should be purple, 20 units away from the camera, centered horizontally, above the red sphere, and circular.
		  Since this object has a the same z-value as the red sphere, it should be smaller due to having a smaller radius.

		> A plane object with R=0, G=0, B=1.0, centered at 0, 1, 0 with a normal of 0, 1, 0.
		  This object should be blue and pointing directly upward due to its normal.  It should also span the bottom half of the image and
		  cover the bottom halves of the red and green spheres.
		

	2.  Open the output file using a program that can read .ppm P3 files.
	    A correctly written output file will appear as follows:
		> A small purple circle that is centered horizontally, cut off at the top, and located at the top of the image.
		> A red circle that is centered and cut in half.
		> A green circle that is to the right, cut in half, and partially blocks the red circle.
		> A blue plane covering the bottom half of the image and bisects the green and red circles.

Invalid inputs and file contents will close the program. 
This program is designed to use eight bits per color channel.