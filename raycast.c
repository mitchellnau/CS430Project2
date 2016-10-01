/* Project 2 *
 * Mitchell Hewitt*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//data type to store pixel rgb values
typedef struct Pixel{
    unsigned char r, g, b;
} Pixel;

FILE* inputfp;
FILE* outputfp;
int width, height, maxcv; //global variables to store header information


//This function writes data from the pixel buffer passed into the function to the output file in ascii.
int write_p3(Pixel* image){
    fprintf(outputfp, "%c%c\n", 'P', '3'); //write out the file header P type
    fprintf(outputfp, "%d %d\n", width, height); //write the width and the height
    fprintf(outputfp, "%d\n", maxcv); //write the max color value
    int i;
    for(i = 0; i < width*height; i++){  //write each pixel in the image to the output file
        fprintf(outputfp, "%d\n%d\n%d\n", image[i*sizeof(Pixel)].r, //in ascii
                image[i*sizeof(Pixel)].g,
                image[i*sizeof(Pixel)].b);
    }
    return 1;
};




int main(int argc, char* argv[]){
    if(argc != 5){
        fprintf(stderr, "Error: Insufficient parameter amount.\nProper input: width height input_filename.json output_filename.ppm\n\n", 001);
        exit(1); //exit the program if there are insufficient arguments
    }
    //echo the command line arguments
    printf("Args: %s\n", argv[0]);
    printf("Args: %s\n", argv[1]);
    printf("Args: %s\n", argv[2]);
    printf("Args: %s\n", argv[3]);
    printf("Args: %s\n", argv[4]);

    inputfp = fopen(argv[3], "r"); //open input to read
    if (inputfp == 0){
        fprintf(stderr, "Error: Input file \"%s\" could not be opened.\n", argv[2], 002);
        exit(1); //if the file cannot be opened, exit the program
    }
    outputfp = fopen(argv[4], "wb"); //open output to write to binary
    if (outputfp == 0){
        fprintf(stderr, "Error: Output file \"%s\" could not be opened.\n", argv[3], 003);
        exit(1); //if the file cannot be opened, exit the program
    }
    width = argv[1][0];
    height = argv[2][0];
    Pixel* data = malloc(sizeof(Pixel)*width*height*3); //allocate memory to hold all of the pixel data
    //write_p3(&data[0]);
    fclose(outputfp); //close the output file
    fclose(inputfp);  //close the input file
    printf("closing...");
    return(0);
}
