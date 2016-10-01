/* Project 2 *
 * Mitchell Hewitt*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//data type to store pixel rgb values
typedef struct Pixel{
    unsigned char r, g, b;
} Pixel;

typedef struct {
  int kind; // 0 = sphere, 1 = plane
  double color[3];
  union {
    struct {
      double center[3];
      double radius;
    } sphere;
    struct {
      double center[3];
      double normal[3];
    } plane;
  };
} Object;

FILE* inputfp;
FILE* outputfp;
int width, height, maxcv; //global variables to store header information
int line = 1;

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
}

// next_c() wraps the getc() function and provides error checking and line
// number maintenance
int next_c(FILE* json){
    int c = fgetc(json);
#ifdef DEBUG
    printf("next_c: '%c'\n", c);
#endif
    if (c == '\n'){
        line += 1;
    }
    if (c == EOF){
        fprintf(stderr, "Error: Unexpected end of file on line number %d.\n", line);
        exit(1);
    }
    return c;
}


// expect_c() checks that the next character is d.  If it is not it emits
// an error.
void expect_c(FILE* json, int d){
    int c = next_c(json);
    if (c == d) return;
    fprintf(stderr, "Error: Expected '%c' on line %d.\n", d, line);
    exit(1);
}


// skip_ws() skips white space in the file.
void skip_ws(FILE* json){
    int c = next_c(json);
    while (isspace(c))
    {
        c = next_c(json);
    }
    ungetc(c, json);
}


// next_string() gets the next string from the file handle and emits an error
// if a string can not be obtained.
char* next_string(FILE* json){
    char buffer[129];
    int c = next_c(json);
    if (c != '"'){
        fprintf(stderr, "Error: Expected string on line %d.\n", line);
        exit(1);
    }
    c = next_c(json);
    int i = 0;
    while (c != '"'){
        if (i >= 128){
            fprintf(stderr, "Error: Strings longer than 128 characters in length are not supported.\n");
            exit(1);
        }
        if (c == '\\'){
            fprintf(stderr, "Error: Strings with escape codes are not supported.\n");
            exit(1);
        }
        if (c < 32 || c > 126){
            fprintf(stderr, "Error: Strings may contain only ascii characters.\n");
            exit(1);
        }
        buffer[i] = c;
        i += 1;
        c = next_c(json);
    }
    buffer[i] = 0;
    return strdup(buffer);
}

double next_number(FILE* json){
    double value;
    fscanf(json, "%f", &value);
    // Error check this..
    return value;
}

double* next_vector(FILE* json){
    double* v = malloc(3*sizeof(double));
    expect_c(json, '[');
    skip_ws(json);
    v[0] = next_number(json);
    skip_ws(json);
    expect_c(json, ',');
    skip_ws(json);
    v[1] = next_number(json);
    skip_ws(json);
    expect_c(json, ',');
    skip_ws(json);
    v[2] = next_number(json);
    skip_ws(json);
    expect_c(json, ']');
    return v;
}


void read_scene(char* filename){
    int c;
    FILE* json = fopen(filename, "r");

    if (json == NULL){
        fprintf(stderr, "Error: Could not open file \"%s\"\n", filename);
        exit(1);
    }

    skip_ws(json);

    // Find the beginning of the list
    expect_c(json, '[');

    skip_ws(json);

    // Find the objects

    while (1){
        c = fgetc(json);
        if (c == ']'){
            fprintf(stderr, "Error: This is the worst scene file EVER.\n");
            fclose(json);
            return;
        }
        if (c == '{'){
            skip_ws(json);

            // Parse the object
            char* key = next_string(json);
            if (strcmp(key, "type") != 0){
                fprintf(stderr, "Error: Expected \"type\" key on line number %d.\n", line);
                exit(1);
            }

            skip_ws(json);

            expect_c(json, ':');

            skip_ws(json);

            char* value = next_string(json);

            if (strcmp(value, "camera") == 0){
            }
            else if (strcmp(value, "sphere") == 0){
            }
            else if (strcmp(value, "plane") == 0){
            }
            else{
                fprintf(stderr, "Error: Unknown type, \"%s\", on line number %d.\n", value, line);
                exit(1);
            }

            skip_ws(json);

            while (1){
                // , }
                c = next_c(json);
                if (c == '}'){
                    // stop parsing this object
                    break;
                }
                else if (c == ','){
                    // read another field
                    skip_ws(json);
                    char* key = next_string(json);
                    skip_ws(json);
                    expect_c(json, ':');
                    skip_ws(json);
                    if ((strcmp(key, "width") == 0) ||
                            (strcmp(key, "height") == 0) ||
                            (strcmp(key, "radius") == 0)){
                        double value = next_number(json);
                    }
                    else if ((strcmp(key, "color") == 0) ||
                             (strcmp(key, "position") == 0) ||
                             (strcmp(key, "normal") == 0)){
                        double* value = next_vector(json);
                    }
                    else{
                        fprintf(stderr, "Error: Unknown property, \"%s\", on line %d.\n",
                                key, line);
                        //char* value = next_string(json);
                    }
                    skip_ws(json);
                }
                else{
                    fprintf(stderr, "Error: Unexpected value on line %d\n", line);
                    exit(1);
                }
            }
            skip_ws(json);
            c = next_c(json);
            if (c == ','){
                // noop
                skip_ws(json);
            }
            else if (c == ']'){
                fclose(json);
                return;
            }
            else{
                fprintf(stderr, "Error: Expecting ',' or ']' on line %d.\n", line);
                exit(1);
            }
        }
    }
}


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

    outputfp = fopen(argv[4], "wb"); //open output to write to binary
    if (outputfp == 0){
        fprintf(stderr, "Error: Output file \"%s\" could not be opened.\n", argv[3], 003);
        exit(1); //if the file cannot be opened, exit the program
    }

    width = argv[1][0];
    height = argv[2][0];
    read_scene(argv[3]);
    Pixel* data = malloc(sizeof(Pixel)*width*height*3); //allocate memory to hold all of the pixel data
    //write_p3(&data[0]);
    fclose(outputfp); //close the output file
    fclose(inputfp);  //close the input file
    printf("closing...");
    return(0);
}
