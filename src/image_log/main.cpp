#include "image/image.hpp"

#include <sstream>
#include <iostream>
#include <string>
#include <ctime>

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

// argv[1] = prefix (including path)
// argv[2] = suffix (not including extension)

int main(int argc, char** argv)
{
	FILE* in = stdin;
	FILE* out = stdout;

	bool quit = false;
	auto prev = std::time(NULL);
	int prev_idx = 0;

	int frames_to_write;
	int cycles;
	while (!quit)
	{
		//gotta keep reading so input doesn't get blocked
		auto img = imageRead(in);
	
		if(cycles > 500){	
			auto now = std::time(NULL);
			std::tm* local = std::localtime(&now);
			char date[256];
			char format[256];
			int idx = prev_idx;
			if (static_cast<int>(std::difftime(now, prev)) <= 0)
				idx += 1;
			else idx = 0;
			prev_idx = idx;
			snprintf(format, sizeof(format) / sizeof(char), "%%Y-%%m%%d_%%H-%%M-%%S_%03i", idx);
			strftime(date, sizeof(date) / sizeof(char), format, local);

			cv::imwrite(std::string(argv[1]) + std::string(date) + std::string(argv[2]) + std::string(".jpg"), img);
		}

		int c = fgetc(in);
		if (c == EOF) quit = true;
		
		cycles++;
		
	}
	return 0;
}

