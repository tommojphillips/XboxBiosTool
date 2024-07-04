#ifndef XB_MAIN_H
#define XB_MAIN_H

void printHelp();
int getFilename(char* path);
int parseArgs(int argc, char* argv[]);
int validateArgs();

int main(int argc, char* argv[]);

#endif // !XB_BIOS_MAIN_H