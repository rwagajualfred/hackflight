#pragma once

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

#define abs(x) ((x) > 0 ? (x) : -(x))

#define RAD    (M_PI / 180.0f)

int constrain(int amt, int low, int high);
