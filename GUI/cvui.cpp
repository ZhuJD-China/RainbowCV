#include "cvui.h"

namespace cvui
{
	double DEFAULT_FONT_SCALE = 0.4;
	unsigned int DEFAULT_BUTTON_COLOR = 0x424242;

	// This is an internal namespace with all code
	// that is shared among components/functions
	namespace internal
	{
		bool isMouseButton(cvui_mouse_btn_t& theButton, int theQuery)
		{
			bool aRet = false;

			switch (theQuery)
			{
			case CLICK:
			case UP:
				aRet = theButton.justReleased;
				break;
			case DOWN:
				aRet = theButton.justPressed;
				break;
			case IS_DOWN:
				aRet = theButton.pressed;
				break;
			}

			return aRet;
		}

		void resetMouseButton(cvui_mouse_btn_t& theButton)
		{
			theButton.justPressed = false;
			theButton.justReleased = false;
			theButton.pressed = false;
		}

		void init(const cv::String& theWindowName, int theDelayWaitKey)
		{
			gDefaultContext = theWindowName;
			gCurrentContext = theWindowName;
			gDelayWaitKey = theDelayWaitKey;
			gLastKeyPressed = -1;
		}

		cvui_context_t& getContext(const cv::String& theWindowName)
		{
			if (!theWindowName.empty())
			{
				// Get context in particular
				return gContexts[theWindowName];
			}
			if (!gCurrentContext.empty())
			{
				// No window provided, return currently active context.
				return gContexts[gCurrentContext];
			}
			if (!gDefaultContext.empty())
			{
				// We have no active context, so let's use the default one.
				return gContexts[gDefaultContext];
			}
			// Apparently we have no window at all! <o>
			// This should not happen. Probably cvui::init() was never called.
			error(5, "Unable to read context. Did you forget to call cvui::init()?");
			return gContexts["first"]; // return to make the compiler happy.
		}

		bool bitsetHas(unsigned int theBitset, unsigned int theValue)
		{
			return (theBitset & theValue) != 0;
		}

		void error(int theId, std::string theMessage)
		{
			std::cout << "[CVUI] Fatal error (code " << theId << "): " << theMessage << "\n";
			cv::waitKey(100000);
			exit(-1);
		}

		void updateLayoutFlow(cvui_block_t& theBlock, cv::Size theSize)
		{
			int aValue;

			if (theBlock.type == ROW)
			{
				aValue = theSize.width + theBlock.padding;

				theBlock.anchor.x += aValue;
				theBlock.fill.width += aValue;
				theBlock.fill.height = std::max(theSize.height, theBlock.fill.height);
			}
			else if (theBlock.type == COLUMN)
			{
				aValue = theSize.height + theBlock.padding;

				theBlock.anchor.y += aValue;
				theBlock.fill.height += aValue;
				theBlock.fill.width = std::max(theSize.width, theBlock.fill.width);
			}
		}

		bool blockStackEmpty()
		{
			return gStackCount == -1;
		}

		cvui_block_t& topBlock()
		{
			if (gStackCount < 0)
			{
				error(
					3,
					"You are using a function that should be enclosed by begin*() and end*(), but you probably forgot to call begin*().");
			}

			return gStack[gStackCount];
		}

		cvui_block_t& pushBlock()
		{
			return gStack[++gStackCount];
		}

		cvui_block_t& popBlock()
		{
			// Check if there is anything to be popped out from the stack.
			if (gStackCount < 0)
			{
				error(1, "Mismatch in the number of begin*()/end*() calls. You are calling one more than the other.");
			}

			return gStack[gStackCount--];
		}

		void begin(int theType, cv::Mat& theWhere, int theX, int theY, int theWidth, int theHeight, int thePadding)
		{
			cvui_block_t& aBlock = pushBlock();

			aBlock.where = theWhere;

			aBlock.rect.x = theX;
			aBlock.rect.y = theY;
			aBlock.rect.width = theWidth;
			aBlock.rect.height = theHeight;

			aBlock.fill = aBlock.rect;
			aBlock.fill.width = 0;
			aBlock.fill.height = 0;

			aBlock.anchor.x = theX;
			aBlock.anchor.y = theY;

			aBlock.padding = thePadding;
			aBlock.type = theType;
		}

		void end(int theType)
		{
			cvui_block_t& aBlock = popBlock();

			if (aBlock.type != theType)
			{
				error(
					4,
					"Calling wrong type of end*(). E.g. endColumn() instead of endRow(). Check if your begin*() calls are matched with their appropriate end*() calls.");
			}

			// If we still have blocks in the stack, we must update
			// the current top with the dimensions that were filled by
			// the newly popped block.

			if (!blockStackEmpty())
			{
				cvui_block_t& aTop = topBlock();
				cv::Size aSize;

				// If the block has rect.width < 0 or rect.heigth < 0, it means the
				// user don't want to calculate the block's width/height. It's up to
				// us do to the math. In that case, we use the block's fill rect to find
				// out the occupied space. If the block's width/height is greater than
				// zero, then the user is very specific about the desired size. In that
				// case, we use the provided width/height, no matter what the fill rect
				// actually is.
				aSize.width = aBlock.rect.width < 0 ? aBlock.fill.width : aBlock.rect.width;
				aSize.height = aBlock.rect.height < 0 ? aBlock.fill.height : aBlock.rect.height;

				updateLayoutFlow(aTop, aSize);
			}
		}

		// Find the min and max values of a vector
		void findMinMax(std::vector<double>& theValues, double* theMin, double* theMax)
		{
			std::vector<double>::size_type aSize = theValues.size(), i;
			double aMin = theValues[0], aMax = theValues[0];

			for (i = 0; i < aSize; i++)
			{
				if (theValues[i] < aMin)
				{
					aMin = theValues[i];
				}

				if (theValues[i] > aMax)
				{
					aMax = theValues[i];
				}
			}

			*theMin = aMin;
			*theMax = aMax;
		}

		cvui_label_t createLabel(const std::string& theLabel)
		{
			cvui_label_t aLabel;
			std::stringstream aBefore, aAfter;

			aLabel.hasShortcut = false;
			aLabel.shortcut = 0;
			aLabel.textBeforeShortcut = "";
			aLabel.textAfterShortcut = "";

			for (size_t i = 0; i < theLabel.size(); i++)
			{
				char c = theLabel[i];
				if ((c == '&') && (i < theLabel.size() - 1))
				{
					aLabel.hasShortcut = true;
					aLabel.shortcut = theLabel[i + 1];
					++i;
				}
				else if (!aLabel.hasShortcut)
				{
					aBefore << c;
				}
				else
				{
					aAfter << c;
				}
			}

			aLabel.textBeforeShortcut = aBefore.str();
			aLabel.textAfterShortcut = aAfter.str();

			return aLabel;
		}

		cv::Scalar hexToScalar(unsigned int theColor)
		{
			int aAlpha = (theColor >> 24) & 0xff;
			int aRed = (theColor >> 16) & 0xff;
			int aGreen = (theColor >> 8) & 0xff;
			int aBlue = theColor & 0xff;

			return cv::Scalar(aBlue, aGreen, aRed, aAlpha);
		}

		unsigned int brightenColor(unsigned int theColor, unsigned int theDelta)
		{
			cv::Scalar color = hexToScalar(theColor);
			cv::Scalar delta = hexToScalar(theDelta);
			int aBlue = std::min(0xFF, static_cast<int>(color[0] + delta[0]));
			int aGreen = std::min(0xFF, static_cast<int>(color[1] + delta[1]));
			int aRed = std::min(0xFF, static_cast<int>(color[2] + delta[2]));
			int aAlpha = std::min(0xFF, static_cast<int>(color[3] + delta[3]));
			return (aAlpha << 24) | (aRed << 16) | (aGreen << 8) | aBlue;
		}

		unsigned int darkenColor(unsigned int theColor, unsigned int theDelta)
		{
			cv::Scalar color = hexToScalar(theColor);
			cv::Scalar delta = hexToScalar(theDelta);
			int aBlue = std::max(0, static_cast<int>(color[0] - delta[0]));
			int aGreen = std::max(0, static_cast<int>(color[1] - delta[1]));
			int aRed = std::max(0, static_cast<int>(color[2] - delta[2]));
			int aAlpha = std::max(0, static_cast<int>(color[3] - delta[3]));
			return (aAlpha << 24) | (aRed << 16) | (aGreen << 8) | aBlue;
		}

		uint8_t brightnessOfColor(unsigned int theColor)
		{
			cv::Mat gray;
			cv::Mat rgb(1, 1, CV_8UC3, hexToScalar(theColor));
			cvtColor(rgb, gray, cv::COLOR_BGR2GRAY);
			return gray.at<cv::Vec3b>(0, 0)[0];
		}

		void resetRenderingBuffer(cvui_block_t& theScreen)
		{
			theScreen.rect.x = 0;
			theScreen.rect.y = 0;
			theScreen.rect.width = 0;
			theScreen.rect.height = 0;

			theScreen.fill = theScreen.rect;
			theScreen.fill.width = 0;
			theScreen.fill.height = 0;

			theScreen.anchor.x = 0;
			theScreen.anchor.y = 0;

			theScreen.padding = 0;
		}


		inline long double clamp01(long double value)
		{
			value = value > 1. ? 1. : value;
			value = value < 0. ? 0. : value;
			return value;
		}

		inline void trackbarForceValuesAsMultiplesOfSmallStep(const TrackbarParams& theParams, long double* theValue)
		{
			if (bitsetHas(theParams.options, TRACKBAR_DISCRETE) && theParams.step != 0.)
			{
				long double k = (*theValue - theParams.min) / theParams.step;
				k = static_cast<long double>(cvRound(static_cast<double>(k)));
				*theValue = theParams.min + theParams.step * k;
			}
		}

		inline long double trackbarXPixelToValue(const TrackbarParams& theParams, cv::Rect& theBounding, int thePixelX)
		{
			long double ratio = (thePixelX - static_cast<long double>(theBounding.x + gTrackbarMarginX)) / static_cast<
				long double>(theBounding.width - 2 * gTrackbarMarginX);
			ratio = clamp01(ratio);
			long double value = theParams.min + ratio * (theParams.max - theParams.min);
			return value;
		}

		inline int trackbarValueToXPixel(const TrackbarParams& theParams, cv::Rect& theBounding, long double theValue)
		{
			long double aRatio = (theValue - theParams.min) / (theParams.max - theParams.min);
			aRatio = clamp01(aRatio);
			long double thePixelsX = static_cast<long double>(theBounding.x) + gTrackbarMarginX + aRatio * static_cast<
				long double>(theBounding.width - 2 * gTrackbarMarginX);
			return static_cast<int>(thePixelsX);
		}

		int iarea(int theX, int theY, int theWidth, int theHeight)
		{
			cvui_mouse_t& aMouse = getContext().mouse;

			// By default, return that the mouse is out of the interaction area.
			int aRet = OUT;

			// Check if the mouse is over the interaction area.
			bool aMouseIsOver = cv::Rect(theX, theY, theWidth, theHeight).contains(aMouse.position);

			if (aMouseIsOver)
			{
				if (aMouse.anyButton.pressed)
				{
					aRet = DOWN;
				}
				else
				{
					aRet = OVER;
				}
			}

			// Tell if the button was clicked or not
			if (aMouseIsOver && aMouse.anyButton.justReleased)
			{
				aRet = CLICK;
			}

			return aRet;
		}

		bool button(cvui_block_t& theBlock, int theX, int theY, int theWidth, int theHeight, const cv::String& theLabel,
		            bool theUpdateLayout, double theFontScale, unsigned int theInsideColor)
		{
			// Calculate the space that the label will fill
			cv::Size aTextSize = getTextSize(theLabel, cv::FONT_HERSHEY_SIMPLEX, theFontScale, 1, nullptr);

			// Make the button bit enough to house the label
			cv::Rect aRect(theX, theY, theWidth, theHeight);

			// Render the button according to mouse interaction, e.g. OVER, DOWN, OUT.
			int aStatus = cvui::iarea(theX, theY, aRect.width, aRect.height);
			render::button(theBlock, aStatus, aRect, theFontScale, theInsideColor);
			render::buttonLabel(theBlock, aStatus, aRect, theLabel, aTextSize, theFontScale, theInsideColor);

			// Update the layout flow according to button size
			// if we were told to update.
			if (theUpdateLayout)
			{
				cv::Size aSize(theWidth, theHeight);
				updateLayoutFlow(theBlock, aSize);
			}

			bool aWasShortcutPressed = false;

			//Handle keyboard shortcuts
			if (gLastKeyPressed != -1)
			{
				// TODO: replace with something like strpos(). I think it has better performance.
				auto aLabel = createLabel(theLabel);
				if (aLabel.hasShortcut && (tolower(aLabel.shortcut) == tolower(static_cast<char>(gLastKeyPressed))))
				{
					aWasShortcutPressed = true;
				}
			}

			// Return true if the button was clicked
			return aStatus == CLICK || aWasShortcutPressed;
		}

		bool button(cvui_block_t& theBlock, int theX, int theY, const cv::String& theLabel, double theFontScale,
		            unsigned int theInsideColor)
		{
			// Calculate the space that the label will fill
			cv::Size aTextSize = getTextSize(theLabel, cv::FONT_HERSHEY_SIMPLEX, theFontScale, 1, nullptr);

			// Create a button based on the size of the text. The size of the additional area outside the label depends on the font size.
			return internal::button(theBlock, theX, theY,
			                        aTextSize.width + std::lround(30 * theFontScale / DEFAULT_FONT_SCALE),
			                        aTextSize.height + std::lround(18 * theFontScale / DEFAULT_FONT_SCALE), theLabel,
			                        true, theFontScale, theInsideColor);
		}

		bool button(cvui_block_t& theBlock, int theX, int theY, cv::Mat& theIdle, cv::Mat& theOver, cv::Mat& theDown,
		            bool theUpdateLayout)
		{
			cv::Rect aRect(theX, theY, theIdle.cols, theIdle.rows);
			int aStatus = cvui::iarea(theX, theY, aRect.width, aRect.height);

			switch (aStatus)
			{
			case OUT: render::image(theBlock, aRect, theIdle);
				break;
			case OVER: render::image(theBlock, aRect, theOver);
				break;
			case DOWN: render::image(theBlock, aRect, theDown);
				break;
			}

			// Update the layout flow according to button size
			// if we were told to update.
			if (theUpdateLayout)
			{
				cv::Size aSize(aRect.width, aRect.height);
				updateLayoutFlow(theBlock, aSize);
			}

			// Return true if the button was clicked
			return aStatus == CLICK;
		}

		void image(cvui_block_t& theBlock, int theX, int theY, cv::Mat& theImage)
		{
			cv::Rect aRect(theX, theY, theImage.cols, theImage.rows);

			// TODO: check for render outside the frame area
			render::image(theBlock, aRect, theImage);

			// Update the layout flow according to image size
			cv::Size aSize(theImage.cols, theImage.rows);
			updateLayoutFlow(theBlock, aSize);
		}

		bool checkbox(cvui_block_t& theBlock, int theX, int theY, const cv::String& theLabel, bool* theState,
		              unsigned int theColor, double theFontScale)
		{
			cvui_mouse_t& aMouse = getContext().mouse;
			cv::Rect aRect(theX, theY, 15, 15);
			cv::Size aTextSize = getTextSize(theLabel, cv::FONT_HERSHEY_SIMPLEX, theFontScale, 1, nullptr);
			cv::Rect aHitArea(theX, theY, aRect.width + aTextSize.width + 6, aRect.height);
			bool aMouseIsOver = aHitArea.contains(aMouse.position);

			if (aMouseIsOver)
			{
				render::checkbox(theBlock, OVER, aRect);

				if (aMouse.anyButton.justReleased)
				{
					*theState = !(*theState);
				}
			}
			else
			{
				render::checkbox(theBlock, OUT, aRect);
			}

			render::checkboxLabel(theBlock, aRect, theLabel, aTextSize, theColor, theFontScale);

			if (*theState)
			{
				render::checkboxCheck(theBlock, aRect);
			}

			// Update the layout flow
			cv::Size aSize(aHitArea.width, aHitArea.height);
			updateLayoutFlow(theBlock, aSize);

			return *theState;
		}

		void text(cvui_block_t& theBlock, int theX, int theY, const cv::String& theText, double theFontScale,
		          unsigned int theColor, bool theUpdateLayout)
		{
			cv::Size aTextSize = getTextSize(theText, cv::FONT_HERSHEY_SIMPLEX, theFontScale, 1, nullptr);
			cv::Point aPos(theX, theY + aTextSize.height);

			render::text(theBlock, theText, aPos, theFontScale, theColor);

			if (theUpdateLayout)
			{
				// Add an extra pixel to the height to overcome OpenCV font size problems.
				aTextSize.height += 1;

				updateLayoutFlow(theBlock, aTextSize);
			}
		}

		int counter(cvui_block_t& theBlock, int theX, int theY, int* theValue, int theStep, const char* theFormat,
		            double theFontScale, unsigned int theInsideColor)
		{
			const double scale = theFontScale / DEFAULT_FONT_SCALE;
			cv::Rect aContentArea(std::lround(theX + 22 * scale), theY, std::lround(48 * scale),
			                      std::lround(22 * scale));

			if (internal::button(theBlock, theX, theY, std::lround(22 * scale), std::lround(22 * scale), "-", false,
			                     theFontScale, theInsideColor))
			{
				*theValue -= theStep;
			}

			sprintf_s(gBuffer, theFormat, *theValue);
			render::counter(theBlock, aContentArea, gBuffer, theFontScale);

			if (internal::button(theBlock, aContentArea.x + aContentArea.width, theY, std::lround(22 * scale),
			                     std::lround(22 * scale), "+", false, theFontScale, theInsideColor))
			{
				*theValue += theStep;
			}

			// Update the layout flow
			cv::Size aSize(std::lround(22 * scale) * 2 + aContentArea.width, aContentArea.height);
			updateLayoutFlow(theBlock, aSize);

			return *theValue;
		}

		double counter(cvui_block_t& theBlock, int theX, int theY, double* theValue, double theStep,
		               const char* theFormat, double theFontScale, unsigned int theInsideColor)
		{
			const double scale = theFontScale / DEFAULT_FONT_SCALE;
			cv::Rect aContentArea(std::lround(theX + 22 * scale), theY, 48, std::lround(22 * scale));

			if (internal::button(theBlock, theX, theY, std::lround(22 * scale), std::lround(22 * scale), "-", false,
			                     theFontScale, theInsideColor))
			{
				*theValue -= theStep;
			}

			sprintf_s(gBuffer, theFormat, *theValue);
			render::counter(theBlock, aContentArea, gBuffer, theFontScale);

			if (internal::button(theBlock, aContentArea.x + aContentArea.width, theY, std::lround(22 * scale),
			                     std::lround(22 * scale), "+", false, theFontScale, theInsideColor))
			{
				*theValue += theStep;
			}

			// Update the layout flow
			cv::Size aSize(std::lround(22 * scale) * 2 + aContentArea.width, aContentArea.height);
			updateLayoutFlow(theBlock, aSize);

			return *theValue;
		}

		bool trackbar(cvui_block_t& theBlock, int theX, int theY, int theWidth, long double* theValue,
		              const TrackbarParams& theParams)
		{
			cvui_mouse_t& aMouse = getContext().mouse;
			cv::Rect aContentArea(theX, theY, theWidth, std::lround(45 * theParams.fontScale / DEFAULT_FONT_SCALE));
			long double aValue = *theValue;
			bool aMouseIsOver = aContentArea.contains(aMouse.position);

			render::trackbar(theBlock, aMouseIsOver ? OVER : OUT, aContentArea, static_cast<double>(*theValue),
			                 theParams);

			if (aMouse.anyButton.pressed && aMouseIsOver)
			{
				*theValue = trackbarXPixelToValue(theParams, aContentArea, aMouse.position.x);

				if (bitsetHas(theParams.options, TRACKBAR_DISCRETE))
				{
					trackbarForceValuesAsMultiplesOfSmallStep(theParams, theValue);
				}
			}

			// Update the layout flow
			cv::Size aSize = aContentArea.size();
			updateLayoutFlow(theBlock, aSize);

			return (*theValue != aValue);
		}


		void window(cvui_block_t& theBlock, int theX, int theY, int theWidth, int theHeight, const cv::String& theTitle,
		            double theFontScale)
		{
			cv::Rect aTitleBar(theX, theY, theWidth, std::lround(20 * theFontScale / DEFAULT_FONT_SCALE));
			cv::Rect aContent(theX, theY + aTitleBar.height, theWidth, theHeight - aTitleBar.height);

			render::window(theBlock, aTitleBar, aContent, theTitle, theFontScale);

			// Update the layout flow
			cv::Size aSize(theWidth, theHeight);
			updateLayoutFlow(theBlock, aSize);
		}

		void rect(cvui_block_t& theBlock, int theX, int theY, int theWidth, int theHeight, unsigned int theBorderColor,
		          unsigned int theFillingColor)
		{
			cv::Point aAnchor(theX, theY);
			cv::Rect aRect(theX, theY, theWidth, theHeight);

			aRect.x = aRect.width < 0 ? aAnchor.x + aRect.width : aAnchor.x;
			aRect.y = aRect.height < 0 ? aAnchor.y + aRect.height : aAnchor.y;
			aRect.width = std::abs(aRect.width);
			aRect.height = std::abs(aRect.height);

			render::rect(theBlock, aRect, theBorderColor, theFillingColor);

			// Update the layout flow
			cv::Size aSize(aRect.width, aRect.height);
			updateLayoutFlow(theBlock, aSize);
		}

		void sparkline(cvui_block_t& theBlock, std::vector<double>& theValues, int theX, int theY, int theWidth,
		               int theHeight, unsigned int theColor)
		{
			double aMin, aMax;
			cv::Rect aRect(theX, theY, theWidth, theHeight);
			std::vector<double>::size_type aHowManyValues = theValues.size();

			if (aHowManyValues >= 2)
			{
				findMinMax(theValues, &aMin, &aMax);
				render::sparkline(theBlock, theValues, aRect, aMin, aMax, theColor);
			}
			else
			{
				internal::text(theBlock, theX, theY, aHowManyValues == 0 ? "No data." : "Insufficient data points.",
				               DEFAULT_FONT_SCALE, 0xCECECE, false);
			}

			// Update the layout flow
			cv::Size aSize(theWidth, theHeight);
			updateLayoutFlow(theBlock, aSize);
		}
	} // namespace internal

	// This is an internal namespace with all functions
	// that actually render each one of the UI components
	namespace render
	{
		void text(cvui_block_t& theBlock, const cv::String& theText, cv::Point& thePos, double theFontScale,
		          unsigned int theColor)
		{
			cv::putText(theBlock.where, theText, thePos, cv::FONT_HERSHEY_SIMPLEX, theFontScale,
			            internal::hexToScalar(theColor), 1, CVUI_ANTIALISED);
		}

		void button(cvui_block_t& theBlock, int theState, cv::Rect& theShape, double theFontScale,
		            unsigned int theInsideColor)
		{
			unsigned int brightColor = internal::brightenColor(theInsideColor, 0x505050);
			unsigned int darkColor = internal::darkenColor(theInsideColor, 0x505050);
			unsigned int topLeftColor, bottomRightColor;
			// 3D effect depending on if the button is down or up. Light comes from top left.
			if (theState == OVER || theState == OUT) // button is up
			{
				topLeftColor = brightColor;
				bottomRightColor = darkColor;
			}
			else // button is down
			{
				bottomRightColor = brightColor;
				topLeftColor = darkColor;
			}

			unsigned int insideOverColor = internal::brightenColor(theInsideColor, 0x101010);
			// particularly this is 0x525252 for DEFAULT_BUTTON_COLOR 0x424242
			unsigned int insideOtherColor = internal::darkenColor(theInsideColor, 0x101010);
			// particularly this is 0x323232 for DEFAULT_BUTTON_COLOR 0x424242

			// 3D Outline. Note that cv::rectangle exludes theShape.br(), so we have to also exclude this point when drawing lines with cv::line
			unsigned int thicknessOf3DOutline = static_cast<int>(theFontScale / 0.6);
			// On high DPI displayed we need to make the border thicker. We scale it together with the font size the user chose.
			do
			{
				line(theBlock.where, theShape.br() - cv::Point(1, 1), cv::Point(theShape.tl().x, theShape.br().y - 1),
				     internal::hexToScalar(bottomRightColor));
				line(theBlock.where, theShape.br() - cv::Point(1, 1), cv::Point(theShape.br().x - 1, theShape.tl().y),
				     internal::hexToScalar(bottomRightColor));
				line(theBlock.where, theShape.tl(), cv::Point(theShape.tl().x, theShape.br().y - 1),
				     internal::hexToScalar(topLeftColor));
				line(theBlock.where, theShape.tl(), cv::Point(theShape.br().x - 1, theShape.tl().y),
				     internal::hexToScalar(topLeftColor));
				theShape.x++;
				theShape.y++;
				theShape.width -= 2;
				theShape.height -= 2;
			}
			while (thicknessOf3DOutline--); // we want at least 1 pixel 3D outline, even for very small fonts

			rectangle(theBlock.where, theShape,
			          theState == OUT
				          ? internal::hexToScalar(theInsideColor)
				          : (theState == OVER
					             ? internal::hexToScalar(insideOverColor)
					             : internal::hexToScalar(insideOtherColor)), CVUI_FILLED);
		}

		int putText(cvui_block_t& theBlock, int theState, cv::Scalar aColor, const std::string& theText,
		            const cv::Point& thePosition, double theFontScale)
		{
			double aFontSize = theState == DOWN ? theFontScale - 0.01 : theFontScale;
			cv::Size aSize;

			if (theText != "")
			{
				cv::putText(theBlock.where, theText, thePosition, cv::FONT_HERSHEY_SIMPLEX, aFontSize, aColor, 1,
				            CVUI_ANTIALISED);
				aSize = getTextSize(theText, cv::FONT_HERSHEY_SIMPLEX, aFontSize, 1, nullptr);
			}

			return aSize.width;
		}

		int putTextCentered(cvui_block_t& theBlock, const cv::Point& position, const std::string& text,
		                    double theFontScale)
		{
			auto size = getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, theFontScale, 1, nullptr);
			cv::Point positionDecentered(position.x - size.width / 2, position.y);
			cv::putText(theBlock.where, text, positionDecentered, cv::FONT_HERSHEY_SIMPLEX, theFontScale,
			            cv::Scalar(0xCE, 0xCE, 0xCE), 1, CVUI_ANTIALISED);

			return size.width;
		}

		void buttonLabel(cvui_block_t& theBlock, int theState, cv::Rect theRect, const cv::String& theLabel,
		                 cv::Size& theTextSize, double theFontScale, unsigned int theInsideColor)
		{
			cv::Point aPos(theRect.x + theRect.width / 2 - theTextSize.width / 2,
			               theRect.y + theRect.height / 2 + theTextSize.height / 2);
			const bool buttonIsDark = internal::brightnessOfColor(theInsideColor) < 0x80;
			cv::Scalar aColor = buttonIsDark ? cv::Scalar(0xCE, 0xCE, 0xCE) : cv::Scalar(0x32, 0x32, 0x32);

			auto aLabel = internal::createLabel(theLabel);

			if (!aLabel.hasShortcut)
			{
				putText(theBlock, theState, aColor, theLabel, aPos, theFontScale);
			}
			else
			{
				int aWidth = putText(theBlock, theState, aColor, aLabel.textBeforeShortcut, aPos, theFontScale);
				int aStart = aPos.x + aWidth;
				aPos.x += aWidth;

				std::string aShortcut;
				aShortcut.push_back(aLabel.shortcut);

				aWidth = putText(theBlock, theState, aColor, aShortcut, aPos, theFontScale);
				int aEnd = aStart + aWidth;
				aPos.x += aWidth;

				putText(theBlock, theState, aColor, aLabel.textAfterShortcut, aPos, theFontScale);
				line(theBlock.where, cv::Point(aStart, aPos.y + 3), cv::Point(aEnd, aPos.y + 3), aColor, 1,
				     CVUI_ANTIALISED);
			}
		}

		void image(cvui_block_t& theBlock, cv::Rect& theRect, cv::Mat& theImage)
		{
			theImage.copyTo(theBlock.where(theRect));
		}

		void counter(cvui_block_t& theBlock, cv::Rect& theShape, const cv::String& theValue, double theFontScale)
		{
			rectangle(theBlock.where, theShape, cv::Scalar(0x29, 0x29, 0x29), CVUI_FILLED); // fill
			rectangle(theBlock.where, theShape, cv::Scalar(0x45, 0x45, 0x45)); // border

			cv::Size aTextSize = getTextSize(theValue, cv::FONT_HERSHEY_SIMPLEX, theFontScale, 1, nullptr);

			cv::Point aPos(theShape.x + theShape.width / 2 - aTextSize.width / 2,
			               theShape.y + aTextSize.height / 2 + theShape.height / 2);
			cv::putText(theBlock.where, theValue, aPos, cv::FONT_HERSHEY_SIMPLEX, theFontScale,
			            cv::Scalar(0xCE, 0xCE, 0xCE), 1, CVUI_ANTIALISED);
		}

		void trackbarHandle(cvui_block_t& theBlock, int theState, cv::Rect& theShape, double theValue,
		                    const internal::TrackbarParams& theParams, cv::Rect& theWorkingArea)
		{
			const double scale = theParams.fontScale / DEFAULT_FONT_SCALE;
			cv::Point aBarTopLeft(theWorkingArea.x, theWorkingArea.y + theWorkingArea.height / 2);
			int aBarHeight = 7;

			// Draw the rectangle representing the handle
			int aPixelX = trackbarValueToXPixel(theParams, theShape, theValue);
			int aIndicatorWidth = std::lround(3 * scale);
			int aIndicatorHeight = std::lround(4 * scale);
			cv::Point aPoint1(aPixelX - aIndicatorWidth, aBarTopLeft.y - aIndicatorHeight);
			cv::Point aPoint2(aPixelX + aIndicatorWidth, aBarTopLeft.y + aBarHeight + aIndicatorHeight);
			cv::Rect aRect(aPoint1, aPoint2);

			int aFillColor = theState == OVER ? 0x525252 : 0x424242;

			rect(theBlock, aRect, 0x212121, 0x212121);
			aRect.x += 1;
			aRect.y += 1;
			aRect.width -= 2;
			aRect.height -= 2;
			rect(theBlock, aRect, 0x515151, aFillColor);

			bool aShowLabel = internal::bitsetHas(theParams.options, TRACKBAR_HIDE_VALUE_LABEL) == false;

			// Draw the handle label
			if (aShowLabel)
			{
				cv::Point aTextPos(aPixelX, aPoint2.y + std::lround(11 * scale));
				sprintf_s(internal::gBuffer, theParams.labelFormat.c_str(), theValue);
				putTextCentered(theBlock, aTextPos, internal::gBuffer, theParams.fontScale - 0.1);
			}
		}

		void trackbarPath(cvui_block_t& theBlock, int theState, cv::Rect& theWorkingArea)
		{
			int aBarHeight = 7;
			cv::Point aBarTopLeft(theWorkingArea.x, theWorkingArea.y + theWorkingArea.height / 2);
			cv::Rect aRect(aBarTopLeft, cv::Size(theWorkingArea.width, aBarHeight));

			int aBorderColor = theState == OVER ? 0x4e4e4e : 0x3e3e3e;

			rect(theBlock, aRect, aBorderColor, 0x292929);
			line(theBlock.where, cv::Point(aRect.x + 1, aRect.y + aBarHeight - 2),
			     cv::Point(aRect.x + aRect.width - 2, aRect.y + aBarHeight - 2), cv::Scalar(0x0e, 0x0e, 0x0e));
		}

		void trackbarSteps(cvui_block_t& theBlock, cv::Rect& theShape, const internal::TrackbarParams& theParams,
		                   cv::Rect& theWorkingArea)
		{
			cv::Point aBarTopLeft(theWorkingArea.x, theWorkingArea.y + theWorkingArea.height / 2);
			cv::Scalar aColor(0x51, 0x51, 0x51);

			bool aDiscrete = internal::bitsetHas(theParams.options, TRACKBAR_DISCRETE);
			long double aFixedStep = aDiscrete ? theParams.step : (theParams.max - theParams.min) / 20;

			// TODO: check min, max and step to prevent infinite loop.
			for (long double aValue = theParams.min; aValue <= theParams.max; aValue += aFixedStep)
			{
				int aPixelX = trackbarValueToXPixel(theParams, theShape, aValue);
				cv::Point aPoint1(aPixelX, aBarTopLeft.y);
				cv::Point aPoint2(aPixelX, aBarTopLeft.y - 3);
				line(theBlock.where, aPoint1, aPoint2, aColor);
			}
		}

		void trackbarSegmentLabel(cvui_block_t& theBlock, cv::Rect& theShape, const internal::TrackbarParams& theParams,
		                          long double theValue, cv::Rect& theWorkingArea, bool theShowLabel)
		{
			cv::Scalar aColor(0x51, 0x51, 0x51);
			cv::Point aBarTopLeft(theWorkingArea.x, theWorkingArea.y + theWorkingArea.height / 2);

			int aPixelX = trackbarValueToXPixel(theParams, theShape, theValue);

			cv::Point aPoint1(aPixelX, aBarTopLeft.y);
			cv::Point aPoint2(aPixelX, aBarTopLeft.y - std::lround(8 * theParams.fontScale / DEFAULT_FONT_SCALE));
			line(theBlock.where, aPoint1, aPoint2, aColor);

			if (theShowLabel)
			{
				sprintf_s(internal::gBuffer, theParams.labelFormat.c_str(), theValue);
				cv::Point aTextPos(aPixelX, aBarTopLeft.y - std::lround(11 * theParams.fontScale / DEFAULT_FONT_SCALE));
				putTextCentered(theBlock, aTextPos, internal::gBuffer, theParams.fontScale - 0.1);
			}
		}

		void trackbarSegments(cvui_block_t& theBlock, cv::Rect& theShape, const internal::TrackbarParams& theParams,
		                      cv::Rect& theWorkingArea)
		{
			int aSegments = theParams.segments < 1 ? 1 : theParams.segments;
			long double aSegmentLength = (theParams.max - theParams.min) / static_cast<long double>(aSegments);

			bool aHasMinMaxLabels = internal::bitsetHas(theParams.options, TRACKBAR_HIDE_MIN_MAX_LABELS) == false;

			// Render the min value label
			trackbarSegmentLabel(theBlock, theShape, theParams, theParams.min, theWorkingArea, aHasMinMaxLabels);

			//Draw large steps and labels
			bool aHasSegmentLabels = internal::bitsetHas(theParams.options, TRACKBAR_HIDE_SEGMENT_LABELS) == false;
			// TODO: check min, max and step to prevent infinite loop.
			for (long double aValue = theParams.min; aValue <= theParams.max; aValue += aSegmentLength)
			{
				trackbarSegmentLabel(theBlock, theShape, theParams, aValue, theWorkingArea, aHasSegmentLabels);
			}

			// Render the max value label
			trackbarSegmentLabel(theBlock, theShape, theParams, theParams.max, theWorkingArea, aHasMinMaxLabels);
		}

		void trackbar(cvui_block_t& theBlock, int theState, cv::Rect& theShape, double theValue,
		              const internal::TrackbarParams& theParams)
		{
			cv::Rect aWorkingArea(theShape.x + internal::gTrackbarMarginX, theShape.y,
			                      theShape.width - 2 * internal::gTrackbarMarginX, theShape.height);

			trackbarPath(theBlock, theState, aWorkingArea);

			bool aHideAllLabels = internal::bitsetHas(theParams.options, TRACKBAR_HIDE_LABELS);
			bool aShowSteps = internal::bitsetHas(theParams.options, TRACKBAR_HIDE_STEP_SCALE) == false;

			if (aShowSteps && !aHideAllLabels)
			{
				trackbarSteps(theBlock, theShape, theParams, aWorkingArea);
			}

			if (!aHideAllLabels)
			{
				trackbarSegments(theBlock, theShape, theParams, aWorkingArea);
			}

			trackbarHandle(theBlock, theState, theShape, theValue, theParams, aWorkingArea);
		}

		void checkbox(cvui_block_t& theBlock, int theState, cv::Rect& theShape)
		{
			// Outline
			rectangle(theBlock.where, theShape,
			          theState == OUT ? cv::Scalar(0x63, 0x63, 0x63) : cv::Scalar(0x80, 0x80, 0x80));

			// Border
			theShape.x++;
			theShape.y++;
			theShape.width -= 2;
			theShape.height -= 2;
			rectangle(theBlock.where, theShape, cv::Scalar(0x17, 0x17, 0x17));

			// Inside
			theShape.x++;
			theShape.y++;
			theShape.width -= 2;
			theShape.height -= 2;
			rectangle(theBlock.where, theShape, cv::Scalar(0x29, 0x29, 0x29), CVUI_FILLED);
		}

		void checkboxLabel(cvui_block_t& theBlock, cv::Rect& theRect, const cv::String& theLabel, cv::Size& theTextSize,
		                   unsigned int theColor, double theFontScale)
		{
			cv::Point aPos(theRect.x + theRect.width + 6,
			               theRect.y + theTextSize.height + theRect.height / 2 - theTextSize.height / 2 - 1);
			text(theBlock, theLabel, aPos, theFontScale, theColor);
		}

		void checkboxCheck(cvui_block_t& theBlock, cv::Rect& theShape)
		{
			theShape.x++;
			theShape.y++;
			theShape.width -= 2;
			theShape.height -= 2;
			rectangle(theBlock.where, theShape, cv::Scalar(0xFF, 0xBF, 0x75), CVUI_FILLED);
		}

		void window(cvui_block_t& theBlock, cv::Rect& theTitleBar, cv::Rect& theContent, const cv::String& theTitle,
		            double theFontScale)
		{
			bool aTransparecy = false;
			double aAlpha = 0.3;
			cv::Mat aOverlay;

			// Render the title bar.
			// First the border
			rectangle(theBlock.where, theTitleBar, cv::Scalar(0x4A, 0x4A, 0x4A));
			// then the inside
			theTitleBar.x++;
			theTitleBar.y++;
			theTitleBar.width -= 2;
			theTitleBar.height -= 2;
			rectangle(theBlock.where, theTitleBar, cv::Scalar(0x21, 0x21, 0x21), CVUI_FILLED);

			// Render title text.
			cv::Point aPos(theTitleBar.x + 5, theTitleBar.y + std::lround(12 * theFontScale / DEFAULT_FONT_SCALE));
			cv::putText(theBlock.where, theTitle, aPos, cv::FONT_HERSHEY_SIMPLEX, theFontScale,
			            cv::Scalar(0xCE, 0xCE, 0xCE), 1, CVUI_ANTIALISED);

			// Render the body.
			// First the border.
			rectangle(theBlock.where, theContent, cv::Scalar(0x4A, 0x4A, 0x4A));

			// Then the filling.
			theContent.x++;
			theContent.y++;
			theContent.width -= 2;
			theContent.height -= 2;
			rectangle(theBlock.where, theContent, cv::Scalar(0x31, 0x31, 0x31), CVUI_FILLED);

			if (aTransparecy)
			{
				theBlock.where.copyTo(aOverlay);
				rectangle(aOverlay, theContent, cv::Scalar(0x31, 0x31, 0x31), CVUI_FILLED);
				addWeighted(aOverlay, aAlpha, theBlock.where, 1.0 - aAlpha, 0.0, theBlock.where);
			}
			else
			{
				rectangle(theBlock.where, theContent, cv::Scalar(0x31, 0x31, 0x31), CVUI_FILLED);
			}
		}

		void rect(cvui_block_t& theBlock, cv::Rect& thePos, unsigned int theBorderColor, unsigned int theFillingColor)
		{
			cv::Scalar aBorder = internal::hexToScalar(theBorderColor);
			cv::Scalar aFilling = internal::hexToScalar(theFillingColor);

			bool aHasFilling = aFilling[3] != 0xff;

			if (aHasFilling)
			{
				if (aFilling[3] == 0x00)
				{
					// full opacity
					rectangle(theBlock.where, thePos, aFilling, CVUI_FILLED, CVUI_ANTIALISED);
				}
				else
				{
					cv::Rect aClippedRect = thePos & cv::Rect(cv::Point(0, 0), theBlock.where.size());
					double aAlpha = 1.00 - aFilling[3] / 255;
					cv::Mat aOverlay(aClippedRect.size(), theBlock.where.type(), aFilling);
					addWeighted(aOverlay, aAlpha, theBlock.where(aClippedRect), 1.00 - aAlpha, 0.0,
					            theBlock.where(aClippedRect));
				}
			}

			// Render the border
			rectangle(theBlock.where, thePos, aBorder, 1, CVUI_ANTIALISED);
		}

		void sparkline(cvui_block_t& theBlock, std::vector<double>& theValues, cv::Rect& theRect, double theMin,
		               double theMax, unsigned int theColor)
		{
			std::vector<double>::size_type aSize = theValues.size(), i;
			double aGap, aPosX, aScale = 0, x, y;

			aScale = theMax - theMin;
			aGap = static_cast<double>(theRect.width) / aSize;
			aPosX = theRect.x;

			for (i = 0; i <= aSize - 2; i++)
			{
				x = aPosX;
				y = (theValues[i] - theMin) / aScale * -(theRect.height - 5) + theRect.y + theRect.height - 5;
				cv::Point aPoint1((x), (y));

				x = aPosX + aGap;
				y = (theValues[i + 1] - theMin) / aScale * -(theRect.height - 5) + theRect.y + theRect.height - 5;
				cv::Point aPoint2((x), (y));

				line(theBlock.where, aPoint1, aPoint2, internal::hexToScalar(theColor));
				aPosX += aGap;
			}
		}
	} // namespace render

	void init(const cv::String& theWindowName, int theDelayWaitKey, bool theCreateNamedWindow)
	{
		internal::init(theWindowName, theDelayWaitKey);
		watch(theWindowName, theCreateNamedWindow);
	}

	void init(const cv::String theWindowNames[], size_t theHowManyWindows, int theDelayWaitKey,
	          bool theCreateNamedWindows)
	{
		internal::init(theWindowNames[0], theDelayWaitKey);

		for (size_t i = 0; i < theHowManyWindows; i++)
		{
			watch(theWindowNames[i], theCreateNamedWindows);
		}
	}

	void watch(const cv::String& theWindowName, bool theCreateNamedWindow)
	{
		cvui_context_t aContex;

		if (theCreateNamedWindow)
		{
			cv::namedWindow(theWindowName);
		}

		aContex.windowName = theWindowName;
		aContex.mouse.position.x = 0;
		aContex.mouse.position.y = 0;

		internal::resetMouseButton(aContex.mouse.anyButton);
		internal::resetMouseButton(aContex.mouse.buttons[RIGHT_BUTTON]);
		internal::resetMouseButton(aContex.mouse.buttons[MIDDLE_BUTTON]);
		internal::resetMouseButton(aContex.mouse.buttons[LEFT_BUTTON]);

		internal::gContexts[theWindowName] = aContex;
		cv::setMouseCallback(theWindowName, handleMouse, &internal::gContexts[theWindowName]);
	}

	void context(const cv::String& theWindowName)
	{
		internal::gCurrentContext = theWindowName;
	}

	void imshow(const cv::String& theWindowName, cv::InputArray theFrame)
	{
		update(theWindowName);
		cv::imshow(theWindowName, theFrame);
	}

	int lastKeyPressed()
	{
		return internal::gLastKeyPressed;
	}

	cv::Point mouse(const cv::String& theWindowName)
	{
		return internal::getContext(theWindowName).mouse.position;
	}

	bool mouse(int theQuery)
	{
		return mouse("", theQuery);
	}

	bool mouse(const cv::String& theWindowName, int theQuery)
	{
		cvui_mouse_btn_t& aButton = internal::getContext(theWindowName).mouse.anyButton;
		bool aRet = internal::isMouseButton(aButton, theQuery);

		return aRet;
	}

	bool mouse(int theButton, int theQuery)
	{
		return mouse("", theButton, theQuery);
	}

	bool mouse(const cv::String& theWindowName, int theButton, int theQuery)
	{
		if (theButton != RIGHT_BUTTON && theButton != MIDDLE_BUTTON && theButton != LEFT_BUTTON)
		{
			internal::error(
				6, "Invalid mouse button. Are you using one of the available: cvui::{RIGHT,MIDDLE,LEFT}_BUTTON ?");
		}

		cvui_mouse_btn_t& aButton = internal::getContext(theWindowName).mouse.buttons[theButton];
		bool aRet = internal::isMouseButton(aButton, theQuery);

		return aRet;
	}

	bool button(cv::Mat& theWhere, int theX, int theY, const cv::String& theLabel, double theFontScale,
	            unsigned int theInsideColor)
	{
		internal::gScreen.where = theWhere;
		return internal::button(internal::gScreen, theX, theY, theLabel, theFontScale, theInsideColor);
	}

	bool button(cv::Mat& theWhere, int theX, int theY, int theWidth, int theHeight, const cv::String& theLabel,
	            double theFontScale, unsigned int theInsideColor)
	{
		internal::gScreen.where = theWhere;
		return internal::button(internal::gScreen, theX, theY, theWidth, theHeight, theLabel, true, theFontScale,
		                        theInsideColor);
	}

	bool button(cv::Mat& theWhere, int theX, int theY, cv::Mat& theIdle, cv::Mat& theOver, cv::Mat& theDown)
	{
		internal::gScreen.where = theWhere;
		return internal::button(internal::gScreen, theX, theY, theIdle, theOver, theDown, true);
	}

	void image(cv::Mat& theWhere, int theX, int theY, cv::Mat& theImage)
	{
		internal::gScreen.where = theWhere;
		return internal::image(internal::gScreen, theX, theY, theImage);
	}

	bool checkbox(cv::Mat& theWhere, int theX, int theY, const cv::String& theLabel, bool* theState,
	              unsigned int theColor, double theFontScale)
	{
		internal::gScreen.where = theWhere;
		return internal::checkbox(internal::gScreen, theX, theY, theLabel, theState, theColor, theFontScale);
	}

	void text(cv::Mat& theWhere, int theX, int theY, const cv::String& theText, double theFontScale,
	          unsigned int theColor)
	{
		internal::gScreen.where = theWhere;
		internal::text(internal::gScreen, theX, theY, theText, theFontScale, theColor, true);
	}

	void printf(cv::Mat& theWhere, int theX, int theY, double theFontScale, unsigned int theColor, const char* theFmt,
	            ...)
	{
		va_list aArgs;

		va_start(aArgs, theFmt);
		vsprintf_s(internal::gBuffer, theFmt, aArgs);
		va_end(aArgs);

		internal::gScreen.where = theWhere;
		internal::text(internal::gScreen, theX, theY, internal::gBuffer, theFontScale, theColor, true);
	}

	void printf(cv::Mat& theWhere, int theX, int theY, const char* theFmt, ...)
	{
		va_list aArgs;

		va_start(aArgs, theFmt);
		vsprintf_s(internal::gBuffer, theFmt, aArgs);
		va_end(aArgs);

		internal::gScreen.where = theWhere;
		internal::text(internal::gScreen, theX, theY, internal::gBuffer, DEFAULT_FONT_SCALE, 0xCECECE, true);
	}

	int counter(cv::Mat& theWhere, int theX, int theY, int* theValue, int theStep, const char* theFormat,
	            double theFontScale, unsigned int theInsideColor)
	{
		internal::gScreen.where = theWhere;
		return internal::counter(internal::gScreen, theX, theY, theValue, theStep, theFormat, theFontScale,
		                         theInsideColor);
	}

	double counter(cv::Mat& theWhere, int theX, int theY, double* theValue, double theStep, const char* theFormat,
	               double theFontScale, unsigned int theInsideColor)
	{
		internal::gScreen.where = theWhere;
		return internal::counter(internal::gScreen, theX, theY, theValue, theStep, theFormat, theFontScale,
		                         theInsideColor);
	}

	void window(cv::Mat& theWhere, int theX, int theY, int theWidth, int theHeight, const cv::String& theTitle,
	            double theFontScale)
	{
		internal::gScreen.where = theWhere;
		internal::window(internal::gScreen, theX, theY, theWidth, theHeight, theTitle, theFontScale);
	}

	void rect(cv::Mat& theWhere, int theX, int theY, int theWidth, int theHeight, unsigned int theBorderColor,
	          unsigned int theFillingColor)
	{
		internal::gScreen.where = theWhere;
		internal::rect(internal::gScreen, theX, theY, theWidth, theHeight, theBorderColor, theFillingColor);
	}

	void sparkline(cv::Mat& theWhere, std::vector<double>& theValues, int theX, int theY, int theWidth, int theHeight,
	               unsigned int theColor)
	{
		internal::gScreen.where = theWhere;
		internal::sparkline(internal::gScreen, theValues, theX, theY, theWidth, theHeight, theColor);
	}

	int iarea(int theX, int theY, int theWidth, int theHeight)
	{
		return internal::iarea(theX, theY, theWidth, theHeight);
	}

	void beginRow(cv::Mat& theWhere, int theX, int theY, int theWidth, int theHeight, int thePadding)
	{
		internal::begin(ROW, theWhere, theX, theY, theWidth, theHeight, thePadding);
	}

	void endRow()
	{
		internal::end(ROW);
	}

	void beginColumn(cv::Mat& theWhere, int theX, int theY, int theWidth, int theHeight, int thePadding)
	{
		internal::begin(COLUMN, theWhere, theX, theY, theWidth, theHeight, thePadding);
	}

	void endColumn()
	{
		internal::end(COLUMN);
	}

	void beginRow(int theWidth, int theHeight, int thePadding)
	{
		cvui_block_t& aBlock = internal::topBlock();
		internal::begin(ROW, aBlock.where, aBlock.anchor.x, aBlock.anchor.y, theWidth, theHeight, thePadding);
	}

	void beginColumn(int theWidth, int theHeight, int thePadding)
	{
		cvui_block_t& aBlock = internal::topBlock();
		internal::begin(COLUMN, aBlock.where, aBlock.anchor.x, aBlock.anchor.y, theWidth, theHeight, thePadding);
	}

	void space(int theValue)
	{
		cvui_block_t& aBlock = internal::topBlock();
		cv::Size aSize(theValue, theValue);

		internal::updateLayoutFlow(aBlock, aSize);
	}

	bool button(const cv::String& theLabel, double theFontScale, unsigned int theInsideColor)
	{
		cvui_block_t& aBlock = internal::topBlock();
		return internal::button(aBlock, aBlock.anchor.x, aBlock.anchor.y, theLabel, theFontScale, theInsideColor);
	}

	bool button(int theWidth, int theHeight, const cv::String& theLabel, double theFontScale,
	            unsigned int theInsideColor)
	{
		cvui_block_t& aBlock = internal::topBlock();
		return internal::button(aBlock, aBlock.anchor.x, aBlock.anchor.y, theWidth, theHeight, theLabel, true,
		                        theFontScale, theInsideColor);
	}

	bool button(cv::Mat& theIdle, cv::Mat& theOver, cv::Mat& theDown)
	{
		cvui_block_t& aBlock = internal::topBlock();
		return internal::button(aBlock, aBlock.anchor.x, aBlock.anchor.y, theIdle, theOver, theDown, true);
	}

	void image(cv::Mat& theImage)
	{
		cvui_block_t& aBlock = internal::topBlock();
		return internal::image(aBlock, aBlock.anchor.x, aBlock.anchor.y, theImage);
	}

	bool checkbox(const cv::String& theLabel, bool* theState, unsigned int theColor, double theFontScale)
	{
		cvui_block_t& aBlock = internal::topBlock();
		return internal::checkbox(aBlock, aBlock.anchor.x, aBlock.anchor.y, theLabel, theState, theColor, theFontScale);
	}

	void text(const cv::String& theText, double theFontScale, unsigned int theColor)
	{
		cvui_block_t& aBlock = internal::topBlock();
		internal::text(aBlock, aBlock.anchor.x, aBlock.anchor.y, theText, theFontScale, theColor, true);
	}

	void printf(double theFontScale, unsigned int theColor, const char* theFmt, ...)
	{
		cvui_block_t& aBlock = internal::topBlock();
		va_list aArgs;

		va_start(aArgs, theFmt);
		vsprintf_s(internal::gBuffer, theFmt, aArgs);
		va_end(aArgs);

		internal::text(aBlock, aBlock.anchor.x, aBlock.anchor.y, internal::gBuffer, theFontScale, theColor, true);
	}

	void printf(const char* theFmt, ...)
	{
		cvui_block_t& aBlock = internal::topBlock();
		va_list aArgs;

		va_start(aArgs, theFmt);
		vsprintf_s(internal::gBuffer, theFmt, aArgs);
		va_end(aArgs);

		internal::text(aBlock, aBlock.anchor.x, aBlock.anchor.y, internal::gBuffer, DEFAULT_FONT_SCALE, 0xCECECE, true);
	}

	int counter(int* theValue, int theStep, const char* theFormat, double theFontScale, unsigned int theInsideColor)
	{
		cvui_block_t& aBlock = internal::topBlock();
		return internal::counter(aBlock, aBlock.anchor.x, aBlock.anchor.y, theValue, theStep, theFormat, theFontScale,
		                         theInsideColor);
	}

	double counter(double* theValue, double theStep, const char* theFormat, double theFontScale,
	               unsigned int theInsideColor)
	{
		cvui_block_t& aBlock = internal::topBlock();
		return internal::counter(aBlock, aBlock.anchor.x, aBlock.anchor.y, theValue, theStep, theFormat, theFontScale,
		                         theInsideColor);
	}

	void window(int theWidth, int theHeight, const cv::String& theTitle, double theFontScale)
	{
		cvui_block_t& aBlock = internal::topBlock();
		internal::window(aBlock, aBlock.anchor.x, aBlock.anchor.y, theWidth, theHeight, theTitle, theFontScale);
	}

	void rect(int theWidth, int theHeight, unsigned int theBorderColor, unsigned int theFillingColor)
	{
		cvui_block_t& aBlock = internal::topBlock();
		internal::rect(aBlock, aBlock.anchor.x, aBlock.anchor.y, theWidth, theHeight, theBorderColor, theFillingColor);
	}

	void sparkline(std::vector<double>& theValues, int theWidth, int theHeight, unsigned int theColor)
	{
		cvui_block_t& aBlock = internal::topBlock();
		internal::sparkline(aBlock, theValues, aBlock.anchor.x, aBlock.anchor.y, theWidth, theHeight, theColor);
	}

	void update(const cv::String& theWindowName)
	{
		cvui_context_t& aContext = internal::getContext(theWindowName);

		aContext.mouse.anyButton.justReleased = false;
		aContext.mouse.anyButton.justPressed = false;

		for (int i = LEFT_BUTTON; i <= RIGHT_BUTTON; i++)
		{
			aContext.mouse.buttons[i].justReleased = false;
			aContext.mouse.buttons[i].justPressed = false;
		}

		internal::resetRenderingBuffer(internal::gScreen);

		// If we were told to keep track of the keyboard shortcuts, we
		// proceed to handle opencv event queue.
		if (internal::gDelayWaitKey > 0)
		{
			internal::gLastKeyPressed = cv::waitKey(internal::gDelayWaitKey);
		}

		if (!internal::blockStackEmpty())
		{
			internal::error(
				2,
				"Calling update() before finishing all begin*()/end*() calls. Did you forget to call a begin*() or an end*()? Check if every begin*() has an appropriate end*() call before you call update().");
		}
	}

	void handleMouse(int theEvent, int theX, int theY, int /*theFlags*/, void* theData)
	{
		int aButtons[3] = {LEFT_BUTTON, MIDDLE_BUTTON, RIGHT_BUTTON};
		int aEventsDown[3] = {cv::EVENT_LBUTTONDOWN, cv::EVENT_MBUTTONDOWN, cv::EVENT_RBUTTONDOWN};
		int aEventsUp[3] = {cv::EVENT_LBUTTONUP, cv::EVENT_MBUTTONUP, cv::EVENT_RBUTTONUP};

		auto aContext = static_cast<cvui_context_t*>(theData);

		for (int i = 0; i < 3; i++)
		{
			int aBtn = aButtons[i];

			if (theEvent == aEventsDown[i])
			{
				aContext->mouse.anyButton.justPressed = true;
				aContext->mouse.anyButton.pressed = true;
				aContext->mouse.buttons[aBtn].justPressed = true;
				aContext->mouse.buttons[aBtn].pressed = true;
			}
			else if (theEvent == aEventsUp[i])
			{
				aContext->mouse.anyButton.justReleased = true;
				aContext->mouse.anyButton.pressed = false;
				aContext->mouse.buttons[aBtn].justReleased = true;
				aContext->mouse.buttons[aBtn].pressed = false;
			}
		}

		aContext->mouse.position.x = theX;
		aContext->mouse.position.y = theY;
	}
} // namespace cvui
