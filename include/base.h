/* base.h
*
*  MIT License
*
*  Copyright (c) 2023-2025 awawa-dev
*
*  https://github.com/awawa-dev/HyperSerialPico
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is
*  furnished to do so, subject to the following conditions:
*
*  The above copyright notice and this permission notice shall be included in all
*  copies or substantial portions of the Software.

*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*  SOFTWARE.
 */

#ifndef BASE_H
#define BASE_H

#include <vector>

class Base
{
	// LED strip number
	int ledsNumber = 0;

	// Should I use sk6812p instead?
	std::vector<LED_DRIVER*> ledStrips;
	std::vector<int> ledCounts = {240, 129};

	// frame is set and ready to render
	bool readyToRender = false;

	public:
		// static data buffer for the loop
		volatile uint8_t buffer[MAX_BUFFER + 1] = {0};
		// handle to tasks
		TaskHandle_t processDataHandle = nullptr;
		TaskHandle_t processSerialHandle = nullptr;
		// semaphore to synchronize them
		semaphore_t serialSemaphore;
		semaphore_t receiverSemaphore;
		// current queue position
		volatile int queueCurrent = 0;
		// queue end position
		volatile int queueEnd = 0;

		inline int getLedCount()
		{
			int sum = 0;
			for (int ledCount : ledCounts) {
				sum += ledCount;
			}
			return sum;
		}

		void initializeLedStrips()
		{
			for (LED_DRIVER* ledStrip : ledStrips) {
				delete ledStrip;
			}
			ledStrips.clear();

			for (int ledCount : ledCounts) {
				LED_DRIVER* ledStrip = new LED_DRIVER(ledCount, DATA_PIN);
				ledStrips.push_back(ledStrip);
			}
		}

		inline int getLedStripCount()
		{
			return ledStrips.size();
		}

		inline LED_DRIVER* getLedStrip(int index)
		{
			if (index < 0 || index >= ledStrips.size()) {
				return nullptr;
			}
			return ledStrips[index];
		}

		inline int getLedStripIndexByPixel(int pixelIndex) {
			int sum = 0;
			for (int i = 0; i < ledCounts.size(); i++) {
				sum += ledCounts[i];
				if (pixelIndex < sum) {
					return i;
				}
			}
			return -1;
		}

		/**
		 * @brief Check if there is already prepared frame to display
		 *
		 * @return true
		 * @return false
		 */
		inline bool hasLateFrameToRender()
		{
			return readyToRender;
		}

		inline void dropLateFrame()
		{
			readyToRender = false;
		}

		inline void renderLeds(bool newFrame)
		{
			if (newFrame)
				readyToRender = true;

			LED_DRIVER* firstLedStrip = getLedStrip(0);
			
			if (readyToRender &&
				(firstLedStrip != nullptr && firstLedStrip->isReadyBlocking()))
			{
				statistics.increaseShow();
				readyToRender = false;

				// if (getLedStripCount() > 1) {
					// firstLedStrip->renderAllLanes();
				// } else {
				// 	// render only the first strip
					firstLedStrip->renderSingleLane();
				// }
			}
		}

		inline bool setStripPixel(uint16_t pixelIndex, ColorDefinition &inputColor)
		{
			// return true if there is another pixel after this one

			if (pixelIndex < getLedCount())
			{
				// figure out which strip it's in
				int stripIndex = getLedStripIndexByPixel(pixelIndex);
				if (stripIndex < 0) {
					return false;
				}

				LED_DRIVER* ledStrip = getLedStrip(stripIndex);
				if (ledStrip == nullptr) {
					return false;
				}

				// set the pixel
				ledStrip->SetPixel(pixelIndex - ledCounts[stripIndex], inputColor);

				// FIXME: Reverse
				// #if defined(SECOND_SEGMENT_REVERSED)
				// ledStrip2->SetPixel(ledsNumber - pixelIndex - 1, inputColor);
			}

			// return true if the pixel is not the last one
			return (pixelIndex + 1 < getLedCount());
		}
} base;

#endif