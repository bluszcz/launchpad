#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <lo/lo.h>
#include <string.h>

#include "liblaunchpad.h"

void error_handler(int num, const char *msg, const char *path);

int generic_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data);

int matrix_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data);

int scene_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data);

int ctrl_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data);

int reset_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data);

int dest_handler(const char *path, const char *types, lo_arg **argv, int argc, void *data, void *user_data);

void* lp2osc();

void* osc2lp();

int main(unsigned int argc, char* argv[]);
