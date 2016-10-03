/* Project 2 *
 * Mitchell Hewitt*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

//data type to store pixel rgb values
typedef struct Pixel{
    unsigned char r, g, b;
} Pixel;

typedef struct{
    int kind; // 0 = camera, 1 = sphere, 2 = plane
    double color[3];
    union{
        struct{
            double center[3];
            double width;
            double height;
        } camera;
        struct{
            double center[3];
            double radius;
        } sphere;
        struct{
            double center[3];
            double normal[3];
        } plane;
    };
} Object;

FILE* inputfp;
FILE* outputfp;
int pwidth, pheight, maxcv; //global variables to store header information
int line = 1;

//This function writes data from the pixel buffer passed into the function to the output file in ascii.
int write_p3(Pixel* image){
    fprintf(outputfp, "%c%c\n", 'P', '3'); //write out the file header P type
    fprintf(outputfp, "%d %d\n", pwidth, pheight); //write the width and the height
    fprintf(outputfp, "%d\n", maxcv); //write the max color value
    int i;
    for(i = 0; i < pwidth*pheight; i++){  //write each pixel in the image to the output file
        fprintf(outputfp, "%d\n%d\n%d\n", image[i*sizeof(Pixel)].r, //in ascii
                image[i*sizeof(Pixel)].g,
                image[i*sizeof(Pixel)].b);
    }
    return 1;
}

static inline double sqr(double v) {
  return v*v;
}

static inline void normalize(double* v) {
  double len = sqrt(sqr(v[0]) + sqr(v[1]) + sqr(v[2]));
  v[0] /= len;
  v[1] /= len;
  v[2] /= len;
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
    char* returnString = malloc(sizeof(buffer));
    strcpy(returnString, buffer);
    return returnString;
}

double next_number(FILE* json){
    double value;
    int f = fscanf(json, "%lf", &value);
    if (f == 1) return value;
    fprintf(stderr, "Error: Expected number on line %d.\n", line);
    exit(1);
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


int read_scene(char* filename, Object* objects){
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
    int i = 0;
    while (1){
        c = fgetc(json);
        if (c == ']'){
            fprintf(stderr, "Error: This is the worst scene file EVER.\n");
            fclose(json);
            return -1;
        }
        if (c == '{'){
            skip_ws(json);
            Object temp;

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
                    temp.kind = 0;
            }
            else if (strcmp(value, "sphere") == 0){
                    temp.kind = 1;
            }
            else if (strcmp(value, "plane") == 0){
                    temp.kind = 2;
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
                        if(temp.kind == 0 && (strcmp(key, "width") == 0)){
                                temp.camera.width = value;
                        }else if(temp.kind == 0 && (strcmp(key, "height") == 0)){
                                temp.camera.height = value;
                        }else if(temp.kind == 1 && (strcmp(key, "radius") == 0)){
                                temp.sphere.radius = value;
                        }else{
                            fprintf(stderr, "Error: Non-camera/sphere object has attribute width or height or radius line number %d.\n", line);
                            exit(1);
                        }
                        if(temp.kind == 0){ //camera assumed to be at 0,0,0
                                temp.camera.center[0] = 0.0;
                                temp.camera.center[1] = 0.0;
                                temp.camera.center[2] = 0.0;
                        }
                    }
                    else if ((strcmp(key, "color") == 0) ||
                             (strcmp(key, "position") == 0) ||
                             (strcmp(key, "normal") == 0)){
                        double* value = next_vector(json);
                        //memcpy(dest, src, sizeof (mytype) * rows * coloumns);
                        if(temp.kind == 1 && (strcmp(key, "color") == 0)){
                                temp.color[0] = value[0];
                                temp.color[1] = value[1];
                                temp.color[2] = value[2];
                        }else if(temp.kind == 1 && (strcmp(key, "position") == 0)){
                                //printf("pos: %f %f %f\n", value[0], value[1], value[2]);
                                temp.sphere.center[0] = value[0];
                                temp.sphere.center[1] = value[1];
                                temp.sphere.center[2] = value[2];
                        }else if(temp.kind == 2 && (strcmp(key, "color") == 0)){
                                temp.color[0] = value[0];
                                temp.color[1] = value[1];
                                temp.color[2] = value[2];
                        }else if(temp.kind == 2 && (strcmp(key, "position") == 0)){
                                temp.plane.center[0] = value[0];
                                temp.plane.center[1] = value[1];
                                temp.plane.center[2] = value[2];
                        }else if(temp.kind == 2 && (strcmp(key, "normal") == 0)){
                                temp.plane.normal[0] = value[0];
                                temp.plane.normal[1] = value[1];
                                temp.plane.normal[2] = value[2];
                        }else if (temp.kind == 0 && (strcmp(key, "position") == 0)){
                                temp.camera.center[0] = value[0];
                                temp.camera.center[1] = value[1];
                                temp.camera.center[2] = value[2];

                        }else{
                            if(temp.kind == 0){
                                    fprintf(stderr, "Error: Camera object has non-position attribute on line %d.\n", line);
                                    exit(1);
                            }else if(temp.kind == 1){
                                    fprintf(stderr, "Error: Sphere object has non-color/position attribute on line %d.\n", line);
                                    exit(1);
                            }else{
                                    fprintf(stderr, "Error: Plane object has non-position/color/normal attribute on line %d.\n", line);
                                    exit(1);
                            }
                        }
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

            //printf("Offset: %d \n", i*sizeof(Object));

            *(objects+i*sizeof(Object)) = temp;
            printf("Kind: %d \n", objects[i*sizeof(Object)].kind);
            i++;


            if (c == ','){
                // noop
                skip_ws(json);
            }
            else if (c == ']'){
                fclose(json);
                return i;
            }
            else{
                fprintf(stderr, "Error: Expecting ',' or ']' on line %d.\n", line);
                exit(1);
            }
        }
    }
}

double sphere_intersection(double* Ro, double* Rd,
			     double* C, double r) {
  double a = (sqr(Rd[0]) + sqr(Rd[1]) + sqr(Rd[2]));
  double b = (2 * (Ro[0] * Rd[0] - Rd[0] * C[0] + Ro[1] * Rd[1] - Rd[1] * C[1] + Ro[2] * Rd[2] - Rd[2] * C[2]));
  double c = sqr(Ro[0]) - 2*Ro[0]*C[0] + sqr(C[0]) + sqr(Ro[1]) - 2*Ro[1]*C[1] + sqr(C[1]) + sqr(Ro[2]) - 2*Ro[2]*C[2] + sqr(C[2]) - sqr(r);

  double det = sqr(b) - 4 * a * c;
  if (det < 0) return -1;

  det = sqrt(det);

  double t0 = (-b - det) / (2*a);
  if (t0 > 0) return t0;

  double t1 = (-b + det) / (2*a);
  if (t1 > 0) return t1;

  return -1;
}

double plane_intersection(double* Ro, double* Rd,
                          double* C, double* N){
  double t, d;
  //t = -(AX0 + BY0 + CZ0 + D) / (AXd + BYd + CZd);
  //D = distance from the origin to the plane
  d = sqrt(sqr(C[0]-Ro[0])+sqr(C[1]-Ro[1])+sqr(C[2]-Ro[2]));
  t = -(N[0]*Ro[0] + N[1]*Ro[1] + N[2]*Ro[2] + d) / (N[0]*Rd[0] + N[1]*Rd[1] + N[2]*Rd[2]);
  //printf("Plane intersection: %d\n", t);
  return t;
}

int main(int argc, char* argv[]){
    if(argc != 5){
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
    if (outputfp == 0){
        fprintf(stderr, "Error: Output file \"%s\" could not be opened.\n", argv[3]);
        exit(1); //if the file cannot be opened, exit the program
    }

    Object* objects = malloc(sizeof(Object*)*128*80);

    pwidth = atoi(argv[1]);
    pheight = atoi(argv[2]);
    if(pwidth == 0){
        fprintf(stderr, "Error: Input width '%d' cannot be zero.\n", pwidth);
        exit(1);
    }
    if(pheight == 0){
        fprintf(stderr, "Error: Input height '%d' cannot be zero.\n", pheight);
        exit(1);
    }
    int numOfObjects = read_scene(argv[3], &objects[0]);
    printf("Object #: %d\n", numOfObjects);
    //objects[numOfObjects] = NULL;
    Pixel* data = malloc(sizeof(Pixel)*pwidth*pheight*3); //allocate memory to hold all of the pixel data


    //printf("Object #: %f\n", objects[1*sizeof(Object)].sphere.center[2]);

    double cx, cy, h, w;
    cx = 0;
    cy = 0;
    h = 1;
    w = 1;
    int i;
    int found = 0;
    for (i=0; i < numOfObjects; i += 1){
        double t = 0;
        if(objects[i*sizeof(Object)].kind == 0){
            w = objects[i*sizeof(Object)].camera.width;
            h = objects[i*sizeof(Object)].camera.height;
            cx = objects[i*sizeof(Object)].camera.center[0];
            cy = objects[i*sizeof(Object)].camera.center[1];
            found = 1;
            break;
        }
    }
    if(found != 1){
        fprintf(stderr, "Error: A camera object was not found in the input json file.\n\tUsing default camera position: (%f,%f)\n\tUsing default camera width: %f\n\tUsing default camera height: %f\n", cx, cy, w, h);
    }

    int M = pheight;
    int N = pwidth;

    double pixheight = h / M;
    double pixwidth = w / N;

    int y, x;

    for (y = 0; y < M; y += 1){
        for (x = 0; x < N; x += 1){
            double Ro[3] = {0, 0, 0};
            // Rd = normalize(P - Ro)
            double Rd[3] ={
                cx - (w/2) + pixwidth * (x + 0.5),
                cy - (h/2) + pixheight * (y + 0.5),
                1
            };
            normalize(Rd);

            double best_t = INFINITY;
            int best_t_i;
            for (i=0; i < numOfObjects; i += 1){
                double t = 0;

                switch(objects[i*sizeof(Object)].kind){
                case 0:
                    break;
                case 1:
                    t = sphere_intersection(Ro, Rd,
                                            objects[i*sizeof(Object)].sphere.center,
                                            objects[i*sizeof(Object)].sphere.radius);
                    break;
                case 2:
                    t = plane_intersection(Ro, Rd,
                                           objects[i*sizeof(Object)].plane.center,
                                           objects[i*sizeof(Object)].plane.normal);
                    break;
                default:
                    // Horrible error
                    exit(1);
                }
                if (t > 0 && t < best_t){
                        best_t = t;
                        best_t_i = i;
                }
            }
            if (best_t > 0 && best_t != INFINITY){
                //printf("#");
                Pixel temporary;
                temporary.r = (int)(objects[best_t_i*sizeof(Object)].color[0]*255);
                temporary.g = (int)(objects[best_t_i*sizeof(Object)].color[1]*255);
                temporary.b = (int)(objects[best_t_i*sizeof(Object)].color[2]*255);
                *(data+(sizeof(Pixel)*pheight*pwidth)-(y+1)*pheight*sizeof(Pixel)+x*sizeof(Pixel)) = temporary;
            }
            else{
                //printf(".");
                Pixel temporary;
                temporary.r = 0;
                temporary.g = 0;
                temporary.b = 0;
                *(data+(sizeof(Pixel)*pheight*pwidth)-(y+1)*pheight*sizeof(Pixel)+x*sizeof(Pixel)) = temporary;
            }

        }
        //printf("\n");
    }
    maxcv = 255;
    write_p3(&data[0]);
    fclose(outputfp); //close the output file
    fclose(inputfp);  //close the input file
    printf("closing...");
    return(0);
}


/*BUGS TO FIX:
error checking
small image breaking*/
