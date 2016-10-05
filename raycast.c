/* Project 2 *
 * Mitchell Hewitt*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

//data type to store pixel rgb values
typedef struct Pixel
{
    unsigned char r, g, b;
} Pixel;

typedef struct
{
    int kind; // 0 = camera, 1 = sphere, 2 = plane
    double color[3];
    union
    {
        struct
        {
            double center[3];
            double width;
            double height;
        } camera;
        struct
        {
            double center[3];
            double radius;
        } sphere;
        struct
        {
            double center[3];
            double normal[3];
        } plane;
    };
} Object;

FILE* outputfp;
int pwidth, pheight, maxcv; //global variables to store p3 header information
int line = 1;               //global variable to store line of json file currently being parsed

//This function writes data from the pixel buffer passed into the function to the output file in ascii.
int write_p3(Pixel* image)
{
    fprintf(outputfp, "%c%c\n", 'P', '3'); //write out the file header P type
    fprintf(outputfp, "%d %d\n", pwidth, pheight); //write the width and the height
    fprintf(outputfp, "%d\n", maxcv); //write the max color value
    int i;
    for(i = 0; i < pwidth*pheight; i++)   //write each pixel in the image to the output file
    {
        fprintf(outputfp, "%d\n%d\n%d\n", image[i*sizeof(Pixel)].r, //in ascii
                image[i*sizeof(Pixel)].g,
                image[i*sizeof(Pixel)].b);
    }
    return 1;
}

//This function returns the input value square
static inline double sqr(double v)
{
    return v*v;
}

//this function normalizes the input vector
static inline void normalize(double* v)
{
    double len = sqrt(sqr(v[0]) + sqr(v[1]) + sqr(v[2]));
    v[0] /= len;
    v[1] /= len;
    v[2] /= len;
}

// next_c() wraps the getc() function and provides error checking and line
// number maintenance
int next_c(FILE* json)
{
    int c = fgetc(json);
#ifdef DEBUG
    printf("next_c: '%c'\n", c);
#endif
    if (c == '\n')
    {
        line += 1;
    }
    if (c == EOF)
    {
        fprintf(stderr, "Error: Unexpected end of file on line number %d.\n", line);
        exit(1);
    }
    return c;
}


// expect_c() checks that the next character is d.  If it is not it emits
// an error.
void expect_c(FILE* json, int d)
{
    int c = next_c(json);
    if (c == d) return;
    fprintf(stderr, "Error: Expected '%c' on line %d.\n", d, line);
    exit(1);
}


// skip_ws() skips white space in the file.
void skip_ws(FILE* json)
{
    int c = next_c(json);
    while (isspace(c))
    {
        c = next_c(json);
    }
    ungetc(c, json);
}


// next_string() gets the next string from the file handle and emits an error
// if a string can not be obtained.
char* next_string(FILE* json)
{
    char buffer[129];
    int c = next_c(json);
    if (c != '"')
    {
        fprintf(stderr, "Error: Expected string on line %d.\n", line);
        exit(1);
    }
    c = next_c(json);
    int i = 0;
    while (c != '"')
    {
        if (i >= 128)
        {
            fprintf(stderr, "Error: Strings longer than 128 characters in length are not supported.\n");
            exit(1);
        }
        if (c == '\\')
        {
            fprintf(stderr, "Error: Strings with escape codes are not supported.\n");
            exit(1);
        }
        if (c < 32 || c > 126)
        {
            fprintf(stderr, "Error: Strings may contain only ascii characters.\n");
            exit(1);
        }
        buffer[i] = c;
        i += 1;
        c = next_c(json);
    }
    buffer[i] = 0;
    char* returnString = malloc(sizeof(buffer));
    strcpy(returnString, buffer);
    return returnString;
}

//This function reads the next number in the input json file and returns that number
//if it was indeed a number.  If a number was not found, it exits the program with an error.
double next_number(FILE* json)
{
    double value;
    int f = fscanf(json, "%lf", &value);
    if (f == 1) return value;
    fprintf(stderr, "Error: Expected number on line %d.\n", line);
    exit(1);
}

//this function reads a three dimensional vector from the input json file.
//Its error handling is inside the expect_c and next_number functions.
//It expects a three dimensional vector which is bookended by brackets where
//each value of the vector is a number and each number is separated by a comma.
double* next_vector(FILE* json)
{
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

//this function takes in a json file and memory to store objects from the file.
//After successfully parsing the json file, it will have stored all objects in
//the json file into the memory passed into the function and will return the
//number of objects it found.
int read_scene(char* filename, Object* objects)
{
    int c;
    FILE* json = fopen(filename, "r");

    if (json == NULL)
    {
        fprintf(stderr, "Error: Could not open file \"%s\"\n", filename);
        exit(1);
    }

    skip_ws(json);
    // Find the beginning of the list
    expect_c(json, '[');
    skip_ws(json);
    // Find the objects
    expect_c(json, '{');
    ungetc('{', json);
    int i = 0;
    while (1)
    {
        c = fgetc(json);
        if (c == ']')          //if the list is empty, the file contains no objects
        {
            fprintf(stderr, "Error: Scene file contains no objects.\n");
            fclose(json);
            return -1;
        }
        if (c == '{')         //if an object is found
        {
            skip_ws(json);
            Object temp;      //temporary variable to store the object

            // Parse the object
            char* key = next_string(json);
            if (strcmp(key, "type") != 0) //object type is the first key of an object expected
            {
                fprintf(stderr, "Error: Expected \"type\" key on line number %d.\n", line);
                exit(1);
            }

            skip_ws(json);
            expect_c(json, ':');        //colon separates key from value in each key-value pair
            skip_ws(json);
            char* value = next_string(json);

            if (strcmp(value, "camera") == 0)
            {
                temp.kind = 0;         //remember that this object is a camera in the temporary Object
            }
            else if (strcmp(value, "sphere") == 0)
            {
                temp.kind = 1;         //remember that this object is a sphere in the temporary Object
            }
            else if (strcmp(value, "plane") == 0)
            {
                temp.kind = 2;         //remember that this object is a plane in the temporary Object
            }
            else                       //if a non-camera/sphere/plane was found as the the type, print an error message and exit
            {
                fprintf(stderr, "Error: Unknown type, \"%s\", on line number %d.\n", value, line);
                exit(1);
            }

            skip_ws(json);

            int w_attribute_counter = 0;
            int h_attribute_counter = 0;
            int r_attribute_counter = 0;
            int c_attribute_counter = 0;
            int p_attribute_counter = 0;
            int n_attribute_counter = 0;

            while (1)         //this loop gets each attribute of an object
            {
                // , }
                c = next_c(json);
                if (c == '}') //curly brace means there are no object properties so break out of the loop
                {
                    // stop parsing this object
                    break;
                }
                else if (c == ',') //there is another object property to be read
                {
                    // read another field
                    skip_ws(json);
                    char* key = next_string(json); //get the key of the property
                    skip_ws(json);
                    expect_c(json, ':');           //key-value pair is separated by a colon
                    skip_ws(json);
                    if ((strcmp(key, "width") == 0) ||    //if the key denotes an decimal number
                            (strcmp(key, "height") == 0) ||
                            (strcmp(key, "radius") == 0))
                    {
                        double value = next_number(json); //get the decimal number and store it in the relevant struct field
                        if(temp.kind == 0 && (strcmp(key, "width") == 0))
                        {
                            temp.camera.width = value;
                            w_attribute_counter++;
                        }
                        else if(temp.kind == 0 && (strcmp(key, "height") == 0))
                        {
                            temp.camera.height = value;
                            h_attribute_counter++;
                        }
                        else if(temp.kind == 1 && (strcmp(key, "radius") == 0))
                        {
                            temp.sphere.radius = value;
                            r_attribute_counter++;
                            if(value <= 0)           //a sphere cannot have a radius of 0 or less so print an error and exit
                            {
                                fprintf(stderr, "Error: Sphere radius cannot be less than or equal to 0 on line %d.\n", line);
                                exit(1);
                            }
                        }
                        else
                        {
                            fprintf(stderr, "Error: Non-camera object has attribute width or height or non-sphere object has a radius on line number %d.\n", line);
                            exit(1);
                        }
                        if(temp.kind == 0)  //camera assumed to be at 0,0,0
                        {
                            temp.camera.center[0] = 0.0;
                            temp.camera.center[1] = 0.0;
                            temp.camera.center[2] = 0.0;
                        }
                    }
                    else if ((strcmp(key, "color") == 0) || //if the key denotes a vector
                             (strcmp(key, "position") == 0) ||
                             (strcmp(key, "normal") == 0))
                    {
                        double* value = next_vector(json); //get the vector and store it in the relevant struct field
                        if(strcmp(key, "color") == 0)
                        {
                            if(value[0] > 1 || value[0] < 0 || //color values must be between 0 and 1
                                    value[1] > 1 || value[1] < 0 || //an error is printed and and the program exits otherwise.
                                    value[2] > 1 || value[2] < 0 )
                            {
                                fprintf(stderr, "Error: Color value is not 0.0 to 1.0 on line number %d.\n", line);
                                exit(1);
                            }
                        }
                        if(temp.kind == 1 && (strcmp(key, "color") == 0))
                        {
                            temp.color[0] = value[0];
                            temp.color[1] = value[1];
                            temp.color[2] = value[2];
                            c_attribute_counter++;
                        }
                        else if(temp.kind == 1 && (strcmp(key, "position") == 0))
                        {
                            temp.sphere.center[0] = value[0];
                            temp.sphere.center[1] = value[1];
                            temp.sphere.center[2] = value[2];
                            p_attribute_counter++;
                        }
                        else if(temp.kind == 2 && (strcmp(key, "color") == 0))
                        {
                            temp.color[0] = value[0];
                            temp.color[1] = value[1];
                            temp.color[2] = value[2];
                            c_attribute_counter++;
                        }
                        else if(temp.kind == 2 && (strcmp(key, "position") == 0))
                        {
                            temp.plane.center[0] = value[0];
                            temp.plane.center[1] = value[1];
                            temp.plane.center[2] = value[2];
                            p_attribute_counter++;
                        }
                        else if(temp.kind == 2 && (strcmp(key, "normal") == 0))
                        {
                            temp.plane.normal[0] = value[0];
                            temp.plane.normal[1] = value[1];
                            temp.plane.normal[2] = value[2];
                            n_attribute_counter++;
                        }
                        else if (temp.kind == 0 && (strcmp(key, "position") == 0))
                        {
                            temp.camera.center[0] = value[0];
                            temp.camera.center[1] = value[1];
                            temp.camera.center[2] = value[2];
                            p_attribute_counter++;
                        }
                        else
                        {
                            if(temp.kind == 0) //if the camera has a vector property that is not a position, print an error and exit
                            {
                                fprintf(stderr, "Error: Camera object has non-position attribute on line %d.\n", line);
                                exit(1);
                            }
                            else if(temp.kind == 1) //if the sphere has a vector property that is not a color or position
                            {
                                fprintf(stderr, "Error: Sphere object has non-color/position attribute on line %d.\n", line);
                                exit(1);
                            }
                            else //if the plane has a vector property that is not a color or position or normal, print an error and exit
                            {
                                fprintf(stderr, "Error: Plane object has non-position/color/normal attribute on line %d.\n", line);
                                exit(1);
                            }
                        }
                    }
                    else //if the input property is unknown, tell the user that property is unknown and exit
                    {
                        fprintf(stderr, "Error: Unknown property, \"%s\", on line %d.\n",
                                key, line);
                        //char* value = next_string(json);
                    }
                    skip_ws(json);
                }
                else //if junk was found in the file tell the user where it was found
                {
                    fprintf(stderr, "Error: Unexpected value on line %d\n", line);
                    exit(1);
                }
            }
            //error checking for duplicate object attributes
            if(temp.kind == 0 && (h_attribute_counter != 1 || w_attribute_counter != 1 || c_attribute_counter != 0 ||
                                  n_attribute_counter != 0 || r_attribute_counter != 0))
            {
                fprintf(stderr, "Error: Expecting unique width, height, (or additionally position) attributes for camera object on line %d.\n");
                exit(1);
            }
            if(temp.kind == 1 && (c_attribute_counter != 1 || r_attribute_counter != 1 || p_attribute_counter != 1 ||
                                  h_attribute_counter != 0 || w_attribute_counter != 0 || n_attribute_counter != 0))
            {
                fprintf(stderr, "Error: Expecting unique color, position, or radius attributes for sphere object on line %d.\n", line);
                exit(1);
            }
            if(temp.kind == 2 && (c_attribute_counter != 1 || p_attribute_counter != 1 || n_attribute_counter != 1  ||
                                  h_attribute_counter != 0 || w_attribute_counter != 0 || r_attribute_counter != 0))
            {
                fprintf(stderr, "Error: Expecting unique color, position, or normal attributes for plane object on line %d.\n", line);
                exit(1);
            }
            skip_ws(json);
            c = next_c(json);

            *(objects+i*sizeof(Object)) = temp; //allocate the temporary object into a struct of objects at its corresponding position
            i++; //and increment the index of the current object for the memory that holds the object structs

            if (c == ',') //if there is another object to be parsed
            {
                skip_ws(json);
                char d = next_c(json);
                if(d != '{')  //if there is another object to be parsed, the next char should be a curly brace, and if not print an error and exit
                {
                    fprintf(stderr, "Error: Expecting '{' on line %d.\n", line);
                    exit(1);
                }
                ungetc(d, json); //if the next char was a curly brace, unget it
            }
            else if (c == ']') //if there are no more objects to be parsed, close the file and return the number of objects
            {
                fclose(json);
                return i;
            }
            else //if a list separator or list terminator was not found, print an error and exit
            {
                fprintf(stderr, "Error: Expecting ',' or ']' on line %d.\n", line);
                exit(1);
            }
        }
    }
}

//this function calculates the t-value that the input ray intersects with an object
//based on the sphere's center position and radius that are each passed into the function.
double sphere_intersection(double* Ro, double* Rd,
                           double* C, double r)
{
    double a = (sqr(Rd[0]) + sqr(Rd[1]) + sqr(Rd[2]));
    double b = (2 * (Ro[0] * Rd[0] - Rd[0] * C[0] + Ro[1] * Rd[1] - Rd[1] * C[1] + Ro[2] * Rd[2] - Rd[2] * C[2]));
    double c = sqr(Ro[0]) - 2*Ro[0]*C[0] + sqr(C[0]) + sqr(Ro[1]) - 2*Ro[1]*C[1] + sqr(C[1]) + sqr(Ro[2]) - 2*Ro[2]*C[2] + sqr(C[2]) - sqr(r);

    double det = sqr(b) - 4 * a * c; //use a b and c to calculate the determinant
    if (det < 0) return -1;          //returns -1 if a number not in the viewplane was calculated

    det = sqrt(det);

    double t0 = (-b - det) / (2*a); //find the first t value, which is smaller
    if (t0 > 0) return t0;          //return it if it is positive

    double t1 = (-b + det) / (2*a); //find the larger second t value
    if (t1 > 0) return t1;          //return it if it is positive

    return -1;                      //return -1 if there are no positive points of intersection
}

//this function calculates the t-value of the intersection of an input ray with a plane.
//based on the plane's center and normal values.
double plane_intersection(double* Ro, double* Rd,
                          double* C, double* N)
{
    double t, d;
    //t = -(AX0 + BY0 + CZ0 + D) / (AXd + BYd + CZd);
    //D = distance from the origin to the plane
    d = sqrt(sqr(C[0]-Ro[0])+sqr(C[1]-Ro[1])+sqr(C[2]-Ro[2])); //calculate the d
    t = -(N[0]*Ro[0] + N[1]*Ro[1] + N[2]*Ro[2] + d) / (N[0]*Rd[0] + N[1]*Rd[1] + N[2]*Rd[2]); //calculate the t where the ray intersects the plane
    return t;
}

//this function takes in the number of objects in the input json file, memory where those objects are stored,
//and a buffer to store the data of each pixel.  It then uses the camera information to display the intersections
//of raycasts and the objects those raycasts are hitting to store RGB pixel values for that spot of intersection
//as observed by the camera position.
void store_pixels(int numOfObjects, Object* objects, Pixel* data)
{
    double cx, cy, h, w;
    cx = 0;  //default camera values
    cy = 0;  // ||
    h = 1;   // ||
    w = 1;   // ||
    int i;
    int found = 0; //tell whether a camera is found or not
    for (i=0; i < numOfObjects; i += 1) //get the first camera's x/y positions and width/height
    {
        if(objects[i*sizeof(Object)].kind == 0)
        {
            w = objects[i*sizeof(Object)].camera.width;
            h = objects[i*sizeof(Object)].camera.height;
            cx = objects[i*sizeof(Object)].camera.center[0];
            cy = objects[i*sizeof(Object)].camera.center[1];
            found = 1;
            break;
        }
    }
    if(found != 1) //if a camera was not found in the list of objects, print an error but continue with default camera values
    {
        fprintf(stderr, "Error: A camera object was not found in the input json file.\n\tUsing default camera position: (%f,%f)\n\tUsing default camera width: %f\n\tUsing default camera height: %f\n", cx, cy, w, h);
    }

    int M = pheight; //M is equal to the input command line height
    int N = pwidth;  //N is equal to the input command line width

    double pixheight = h / M; //pixel height and width of the area to be raycasted
    double pixwidth = w / N;

    int y, x; //loop control variables

    printf("calculating intersections and storing intersection pixels...\n");
    for (y = 0; y < M; y += 1)
    {
        for (x = 0; x < N; x += 1)
        {
            double Ro[3] = {0, 0, 0};
            // Rd = normalize(P - Ro)
            double Rd[3] =
            {
                cx - (w/2) + pixwidth * (x + 0.5),
                cy - (h/2) + pixheight * (y + 0.5),
                1
            };
            normalize(Rd);

            double best_t = INFINITY; //find the minimum best t intersection of any object
            int best_t_i; //keep track of the corresponding object's index
            for (i=0; i < numOfObjects; i += 1)
            {
                double t = 0;

                switch(objects[i*sizeof(Object)].kind)
                {
                case 0: //camera has no physical intersections
                    break;
                case 1: //if the object is a sphere, find its minimum intersection
                    t = sphere_intersection(Ro, Rd,
                                            objects[i*sizeof(Object)].sphere.center,
                                            objects[i*sizeof(Object)].sphere.radius);
                    break;
                case 2: //if the object is a plane, find its point of intersection
                    t = plane_intersection(Ro, Rd,
                                           objects[i*sizeof(Object)].plane.center,
                                           objects[i*sizeof(Object)].plane.normal);
                    break;
                default:
                    fprintf(stderr, "Error: Forbidden object struct type located in memory, intersection could not be calculated.\n");
                    exit(1);
                }
                if (t > 0 && t < best_t) //if an object is in front of another object, ensure the front-most object is displayed
                {
                    best_t = t;
                    best_t_i = i;
                }
            }
            if (best_t > 0 && best_t != INFINITY) //if the intersection is in the viewplane and isn't infinity, store its object's color into the buffer
            {
                //at the correct x,y location
                //printf("here. x %d\ty %d\n", x, y);
                Pixel temporary;
                temporary.r = (int)(objects[best_t_i*sizeof(Object)].color[0]*255);
                temporary.g = (int)(objects[best_t_i*sizeof(Object)].color[1]*255);
                temporary.b = (int)(objects[best_t_i*sizeof(Object)].color[2]*255);
                *(data+(sizeof(Pixel)*pheight*pwidth)-(y+1)*pwidth*sizeof(Pixel)+x*sizeof(Pixel)) = temporary;
            }
            else //no point of intersection was found for any object at the given x,y so put black into that x,y pixel into the buffer
            {
                //printf("here. x %3d\ty %3d\n", x, y);
                Pixel temporary;
                temporary.r = 0;
                temporary.g = 0;
                temporary.b = 0;
                *(data+(sizeof(Pixel)*pheight*pwidth)-(y+1)*pwidth*sizeof(Pixel)+x*sizeof(Pixel)) = temporary;
            }

        }
    }
}


int main(int argc, char* argv[])
{
    if(argc != 5)
    {
        fprintf(stderr, "Error: Insufficient parameter amount.\nProper input: width height input_filename.json output_filename.ppm\n\n");
        exit(1); //exit the program if there are insufficient arguments
    }
    //echo the command line arguments
    printf("Arg 0: %s\n", argv[0]);
    printf("Arg 1: %s\n", argv[1]);
    printf("Arg 2: %s\n", argv[2]);
    printf("Arg 3: %s\n", argv[3]);
    printf("Arg 4: %s\n", argv[4]);

    outputfp = fopen(argv[4], "wb"); //open output to write to binary
    if (outputfp == 0)
    {
        fprintf(stderr, "Error: Output file \"%s\" could not be opened.\n", argv[3]);
        exit(1); //if the file cannot be opened, exit the program
    }

    Object* objects = malloc(sizeof(Object)*128);

    pwidth = atoi(argv[1]);
    pheight = atoi(argv[2]);
    if(pwidth <= 0)
    {
        fprintf(stderr, "Error: Input width '%d' cannot be less than or equal to zero.\n", pwidth);
        exit(1);
    }
    if(pheight <= 0)
    {
        fprintf(stderr, "Error: Input height '%d' cannot be less than or equal to zero.\n", pheight);
        exit(1);
    }
    int numOfObjects = read_scene(argv[3], &objects[0]);  //parse the scene and store the number of objects
    printf("# of Objects: %d\n", numOfObjects);           //echo the number of objects
    Pixel* data = malloc(sizeof(Pixel)*pwidth*pheight*3); //allocate memory to hold all of the pixel data


    store_pixels(numOfObjects, &objects[0], &data[0]);    //store the points of ray intersection and that object's color values into a buffer
    maxcv = 255;
    printf("writing to image file...\n");
    int successfulWrite = write_p3(&data[0]);             //write the pixel buffer to the image file
    if(successfulWrite != 1)
    {
        fprintf(stderr, "Error: Failed to properly write to output image file.\n");
        exit(1);
    }
    fclose(outputfp); //close the output file
    printf("closing...");
    return(0);
}
