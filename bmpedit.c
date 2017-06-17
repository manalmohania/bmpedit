#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


/*Converts 4 bytes to an integer value*/
int convert_to_int(unsigned char array[], int start){
	return(array[start] | array[start + 1] << 8 | array[start + 2] << 16 | array[start + 3] << 24);
}

void make_black_and_white(int offset, unsigned char array[], int size, float threshold, int pad, int row_size){
	
	int i, j;
	
	for(i = offset; i < size; i += 3){
		/*ignores the padding bytes*/
		if(((i - offset) % row_size == 0) && i != offset){
			for(j = 0; j < pad; j++){
				array[i + j] = 0;
			}
			i = i + j;
		}
		/*The avg of the BGR values is taken and divided by 255, yielding a quantity between 0 and 1*/
		float sumColour = array[i] + array[i + 1] + array[i + 2];
		float lowered_down  = sumColour / (3 * 255);
		/*If each of the BGR components are set to 255, the pixel colour becomes white*/
		if(lowered_down > threshold){
			array[i] = 255;
			array[i + 1] = 255;
			array[i + 2] = 255;
		}
		/*Similarly, zero for each the BGR values means a black pixel*/
		else{
			array[i] = 0;
			array[i + 1] = 0;
			array[i + 2] = 0;
		}		
	}
}

/*The following function returns 1 if the difference between the colours of pixels is above a certain threshold*/
/*The threshold is as follows
	1. if any of the B, G or R components differs by a value of at least 50
	2. if any two of the B, G or R components differs by a value of at least 25
	3. if all three values differs by at least 10

	I came up with these values after testings on 7 images. These numbers seemed to give the best results.
*/
int diff_is_high(unsigned char array[], int idx1, int idx2){
	if((abs(array[idx1] - array[idx2]) > 60) || (abs(array[idx1 + 1] - array[idx2 + 1]) > 60) || (abs(array[idx1] - array[idx2 + 1]) > 60))
		return 1;
	if(((abs(array[idx1] - array[idx2]) > 40) && (abs(array[idx1 + 1] - array[idx2 + 1]) > 40)) || \
	((abs(array[idx1 + 1] - array[idx2 + 1]) > 40) && (abs(array[idx1 + 2] - array[idx2 + 2]) > 40)) || \
	((abs(array[idx1 + 2] - array[idx2 + 2]) > 40) && (abs(array[idx1] - array[idx2]) > 40)))
		return 1;
	if(((abs(array[idx1] - array[idx2])) > 20) && ((abs(array[idx1 + 1] - array[idx2 + 1])) > 20) && ((abs(array[idx1 + 2] - array[idx2 + 2])) > 20))
		return 1;
	return 0;
}

void detect_edges(int offset, unsigned char array[], int size, int height, int width, unsigned char changed[], int pad, int row_size){
	int i, j;
	
	/*Basic idea: if any of the pixels around a pixel has a difference in colour which is above a certain threshold,
	make the pixel black. If not, change it to white
	*/
	for (i = offset; i < size - 3; i += 3){
	
		if(((i - offset) % row_size == 0) && i != offset){
			for(j = 0; j < pad; j++){
				array[i + j] = 0;
			}
			i = i + j;
		}
		
		/*When dealing with the first row*/
		if(i <= offset + row_size){
			if(diff_is_high(array, i, row_size + i) || diff_is_high(array, i, i + 3)){
				changed[i] = 0;
				changed[i + 1] = 0;
				changed[i + 2] = 0;
			}
			else{
				changed[i] = 255;
				changed[i + 1] = 255;
				changed[i + 2] = 255;
			}
			
		}
		/*The last row*/
		else if(i >= offset + (height - 1) * row_size){
			if(diff_is_high(array, i, i) || diff_is_high(array, i, i + 3)){
				changed[i] = 0;
				changed[i + 1] = 0;
				changed[i + 2] = 0;
			}
			else{
				changed[i] = 255;
				changed[i + 1] = 255;
				changed[i + 2] = 255;
			}
		}
		/*The diff_is_high function is called three times for each pixel otherwise*/
		else{
			if (diff_is_high(array, i, row_size + i) || diff_is_high(array, i, i - row_size) || diff_is_high(array, i, i + 3)){
				changed[i] = 0;
				changed[i + 1] = 0;
				changed[i + 2] = 0;
			}
			else{
				changed[i] = 255;
				changed[i + 1] = 255;
				changed[i + 2] = 255;
			}
		}
	}
}

void mirror(unsigned char array[], unsigned char changed[], int width, int size_of_file, int row_size, int offset){

	int i;
	int j;
	int count = 1; 
	int mirror = ((width + 1) / 2) * 3;
	
	for(i = offset; i < size_of_file;){
	
		/*The first few bytes of each row are copied as they are*/
		if((i-offset) % row_size < mirror){
			changed[i] = array[i];
			i++;
		}
		/*When it crosses half of the width (mirror), the bytes that are opposite to the mirror are written to the index*/
		else if((i - offset) % row_size < (mirror + (width / 2) * 3)){
			for(j = 0; j < 3; j++){
				changed[i] = array[i - count*3];
				i++;
			}
			count += 2;	
		}
		/*make padding bytes zero*/
		else{
			changed[i] = 0;
			i++;
		}	
		/*After one row is covered, reset the count variable to 1*/
		if(count > 2 * (width / 2) - 1)
			count = 1;
	}
	
}

int calculate_padding(int width){
	int padding;
	padding  = (24 * width + 31) / 32;
	if ((4 * padding) % (width * 3) == 0)
		return 0;
	else
		return (4 - ((4 * padding) % (width * 3)));
}

void invert_colour(int offset, unsigned char array[], int size_of_file, int row_size, int pad){
	int i, j;
	
	for(i=offset; i < size_of_file ; i++){
	
	/*Padding Bytes*/
	if(((i - offset) % row_size == 0) && i != offset){
			for(j = 0; j < pad; j++){
				array[i + j] = 0;
			}
			i = i + j;
		}
		/*255 - colour gives the inverted colour*/
		array[i] = 255 - array[i];
	}
}

int is_valid_bmp(unsigned char array[]){
	/*The first two byes must correspond to the BMP signature*/
	if(!(array[0] == 66 && array[1] == 77)){
		printf("I read BMP images only!\n");
		return 0;
	}
	
	if(array[28] != 24){
		printf("Sorry, I only work with 24 bits per pixel BMP images\n");
		return 0;
	}
	return 1;
}

void print_help(){
	printf("Arguments for usage: \n./nameOfOutputFile <option> <-o> <nameOfOutputImage> <nameOfInputImage>\n");
	printf("Possible Options-\n-t (0.0 - 1.0) for threshold filter\n-i for colour inversion\n-m for mirror effect\n-e for edge detection\nOpen README.md if you need further help!\n");
	exit(1);
}


int offset;
int padding;

int main(int argc, char* argv[]){

	/***Checks the validity of the arguments supplied***/
	if(argc < 2 || strcmp(argv[1], "-h") == 0 || (argc < 5 || argc > 7) || ((strcmp(argv[1], "-t") == 0) && argc != 6)){
		print_help();
	}
	
	else if((strcmp(argv[1], "-t") != 0) && argc!=5){
		print_help();
	}
	
	else if((strcmp(argv[1], "-t") != 0) && (strcmp(argv[1], "-i") != 0) && (strcmp(argv[1], "-e") != 0) && (strcmp(argv[1], "-m") != 0)){		
		print_help();
	}
	
	else if(((strcmp(argv[3], "-o") != 0) && (strcmp(argv[1], "-t") == 0)) || ((strcmp(argv[2], "-o") != 0) && (strcmp(argv[1], "-t") != 0))){
		print_help();
	}
		
	/***************************************************/
	
	int i;
	unsigned char buffer[40];
	int size_of_file;
	int width, height;
	int bytes_read1 = 0;
	int bytes_read2;
	int bytes_written;
	int row_size;
	float threshold = 2.0;
	FILE* img;
	FILE* img2;
	FILE* img3;
	int bw = 0;
	
	/*Checks the validity of the threshold parameter*/
	
	if(strcmp(argv[1], "-t") == 0){
		bw = 1;
		threshold = atof(argv[2]);
		if (threshold < 0 || threshold > 1){
			printf("The threshold needs to be between 0 and 1. Set to 0.5\n");
			threshold = 0.5;
		}
	}
	
	/*Opens a new file and reads in the first few bytes to an array.
	 The size of the size of the file is obtained from the header, and a new array of the required size is created.*/
	
	
	if(bw){
		img = fopen(argv[5], "r");
		if (img == NULL){
			printf("Error opening file :( \nEnsure that \n1. You are in the correct directory\n2. File with name %s exists\n", argv[5]);
			exit(1);
		}	
	}
	else{
		img = fopen(argv[4], "r");
		if (img == NULL){
			printf("Error opening file :( \nEnsure that \n1. You are in the correct directory\n2. File with name %s exists\n", argv[4]);
			exit(1);
		}
	}
	
	/*Read in the first 40 bytes to array named buffer*/
	bytes_read1 = fread(buffer, /*sizeof(unsigned char)*/ 1, 40, img);
	if(bytes_read1 < 40)
		printf("Error reading file. \n");
	
	/*Checks if file is valid bmp*/
	if (!(is_valid_bmp(buffer)))
		exit(1);
	

	/*Calculate size of file*/
	size_of_file = convert_to_int(buffer, 2);
	
	fclose(img);

	/*Reopens the file*/
	if(bw)
		img2 = fopen(argv[5], "r");	
	else
		img2 = fopen(argv[4], "r");
		
	unsigned char whole[size_of_file];
	unsigned char wholeDup[size_of_file];
	
	/*Reads into array named whole, whose size is equal to the size of file*/
	
	bytes_read2 = fread(whole, 1, size_of_file, img2);
	if(bytes_read2 < size_of_file)
		printf("Error reading file!!\n");		
	
	width = convert_to_int(whole, 18);
	height = convert_to_int(whole, 22);
	offset = convert_to_int(whole, 10);
	row_size = (24*width + 31)/32;	// note that this not the actual row size; whenever called, it is multiplied by a factor of 4
	
	/*Calculates padding for a given width*/
	padding = calculate_padding(width);
	
	printf("Width: %d\nHeight: %d\n", width, height);
	
	for(i=0; i < size_of_file ; i++)
		wholeDup[i] = whole[i];	// Creates a duplicate of whole for later use
	
	fclose(img2);
	
	if(bw){
		img3 = fopen(argv[4], "w");
		if(img3 == NULL)
			printf("Error opening file\n");
	}
	else{
		img3 = fopen(argv[3], "w");
		if(img3 == NULL)
			printf("Error opening file\n");
	}
	
	if(strcmp(argv[1], "-e") == 0)
		detect_edges(offset, whole, size_of_file, height, width, wholeDup, padding, 4*row_size);
	if(strcmp(argv[1], "-m") == 0)
		mirror(whole, wholeDup, width, size_of_file, 4*row_size, offset);
	if(bw)
		make_black_and_white(offset, wholeDup, size_of_file, threshold, padding, 4*row_size);
	if(strcmp(argv[1], "-i") == 0)
		invert_colour(offset, wholeDup, size_of_file, 4*row_size, padding);
	
	
	bytes_written = fwrite(wholeDup, 1, size_of_file, img3);
	if(bytes_written < size_of_file)
		printf("Error writing to file\n");
	
	fclose(img3);
	
}
