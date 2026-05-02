#pragma once

void setupScale();
bool tareScale();
bool doCalibration(float referenceWeight);
bool setFactor(float newFactor);
bool setOffset(float newOffset);
float updateScale();
float getFactor();
float getOffset();