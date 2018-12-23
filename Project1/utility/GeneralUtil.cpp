#include "GeneralUtil.h"


namespace Util
{
	bool min_max_1dto2dgrid(uint32_t start, uint32_t totalSize, uint32_t gridSize, uint32_t& outWidth, uint32_t& outHeight, uint32_t& outMin, uint32_t& outMax)
	{
		// check that the size is divisible by 2 adn that the start is within the sequential set of numbers which make up the 1d representation of the grid
		if (gridSize % 2 == 0 || start > (totalSize * totalSize) - 1) {
			return false;
		}

		uint32_t row = start / totalSize;
		uint32_t col = start % totalSize;

		// the boundaries for this 2d grid
		uint32_t rowStart = row * totalSize;
		uint32_t rowEnd = (row * totalSize) + (totalSize - 1);
		uint32_t colStart = col;
		uint32_t colEnd = col * totalSize;

		uint8_t delta = (gridSize - 1) / 2;

		// for x axis: check that we aren't going outisde of the left-side row limit and is so adjust the output grid width
		uint32_t width = gridSize;
		if (start - delta < rowStart) {
			uint32_t diff = rowStart - (start - delta);
			width -= diff;
		}

		// check that we aren't outside of the right-side row limit 
		if (start + delta > rowEnd) {
			uint32_t diff = rowEnd - (start + delta);
			width -= diff;
		}

		// now do for the y axis. we will also calculate the min value
		uint32_t height = gridSize;
		uint32_t yMin = start - (col * totalSize);
		if (yMin < colStart) {
			uint32_t diff = colStart - yMin;
			height -= diff;
			outMin = yMin - (delta - diff);
		}
		else {
			outMin = yMin - delta;
		}

		uint32_t yMax = start + (col * totalSize);
		if (yMax > colEnd) {
			uint32_t diff = colEnd - yMax;
			height -= diff;
			outMax = yMax + (delta - diff);
		}
		else {
			outMax = yMax + delta;
		}

		outWidth = width;
		outHeight = height;
		return true;
	}
}
