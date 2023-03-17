/*
 A (very) simple UI lib built on top of OpenCV drawing primitives.

 Usage:

 One (and only one) of your C++ files must define CVUI_IMPLEMENTATION
 before the inclusion of cvui.h to ensure its implementaiton is compiled.

 E.g:

   #define CVUI_IMPLEMENTATION
   #include "cvui.h"

   int main() {
   }

 All other files can include cvui.h without defining CVUI_IMPLEMENTATION.
 
 Use of cvui revolves around calling cvui::init() to initialize the lib, 
 rendering cvui components to a cv::Mat (that you handle yourself) and
 finally showing that cv::Mat on the screen using cvui::imshow(), which
 is cvui's version of cv::imshow(). Alternatively you can use cv::imshow()
 to show things, but in such case you must call cvui::update() yourself
 before calling cv::imshow().
 
 E.g.:

   #include <opencv2/opencv.hpp>
   #define CVUI_IMPLEMENTATION
   #include "cvui.h"

   #define WINDOW1_NAME "Window 1"

   int main() {
     cvui::init(WINDOW1_NAME);
     cv::Mat frame = cv::Mat(cv::Size(400, 200), CV_8UC3);

     while(true) {
       frame = cv::Scalar(49, 52, 49);
       cvui::text(frame, x, y, "Hello world!");

	   cvui::imshow(WINDOW1_NAME, frame);

       if (cv::waitKey(20) == 27) {
         break;
       }
    }
    return 0;
  }
*/

#ifndef _CVUI_H_
#define _CVUI_H_

#include <iostream>
#include <vector>
#include <map>
#include <stdarg.h>

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>

namespace cvui
{
	extern double DEFAULT_FONT_SCALE;
	extern unsigned int DEFAULT_BUTTON_COLOR;
	/**
	 Initializes cvui. You must provide the name of the window where
	 components will be added. It is also possible to tell cvui to handle
	 OpenCV's event queue automatically (by informing a value greater than zero
	 in the `theDelayWaitKey` parameter of the function). In that case, cvui will
	 automatically call `cv::waitKey()` within `cvui::update()`, so you don't
	 have to worry about it. The value passed to `theDelayWaitKey` will be
	 used as the delay for `cv::waitKey()`.
	 
	 \param theWindowName name of the window where the components will be added.
	 \param theDelayWaitKey delay value passed to `cv::waitKey()`. If a negative value is informed (default is `-1`), cvui will not automatically call `cv::waitKey()` within `cvui::update()`, which will disable keyboard shortcuts for all components. If you want to enable keyboard shortcut for components (e.g. using & in a button label), you must specify a positive value for this param.
	 \param theCreateNamedWindow if an OpenCV window named `theWindowName` should be created during the initialization. Windows are created using `cv::namedWindow()`. If this parameter is `false`, ensure you call `cv::namedWindow(WINDOW_NAME)` *before* initializing cvui, otherwise it will not be able to track UI interactions. 
	
	 \sa watch()
	 \sa context()
	*/
	void init(const cv::String& theWindowName, int theDelayWaitKey = -1, bool theCreateNamedWindow = true);

	/**
	 Initialize cvui using a list of names of windows where components will be added.
	 It is also possible to tell cvui to handle OpenCV's event queue automatically
	 (by informing a value greater than zero in the `theDelayWaitKey` parameter of the function).
	 In that case, cvui will automatically call `cv::waitKey()` within `cvui::update()`,
	 so you don't have to worry about it. The value passed to `theDelayWaitKey` will be
	 used as the delay for `cv::waitKey()`.
	
	 \param theWindowNames array containing the name of the windows where components will be added. Those windows will be automatically if `theCreateNamedWindows` is `true`.
	 \param theHowManyWindows how many window names exist in the `theWindowNames` array.
	 \param theDelayWaitKey delay value passed to `cv::waitKey()`. If a negative value is informed (default is `-1`), cvui will not automatically call `cv::waitKey()` within `cvui::update()`, which will disable keyboard shortcuts for all components. If you want to enable keyboard shortcut for components (e.g. using & in a button label), you must specify a positive value for this param.
	 \param theCreateNamedWindows if OpenCV windows named according to `theWindowNames` should be created during the initialization. Windows are created using `cv::namedWindow()`. If this parameter is `false`, ensure you call `cv::namedWindow(WINDOW_NAME)` for all windows *before* initializing cvui, otherwise it will not be able to track UI interactions.
	
	 \sa watch()
	 \sa context()
	*/
	void init(const cv::String theWindowNames[], size_t theHowManyWindows, int theDelayWaitKey = -1,
	          bool theCreateNamedWindows = true);

	/**
	 Track UI interactions of a particular window. This function must be invoked
	 for any window that will receive cvui components. cvui automatically calls `cvui::watch()`
	 for any window informed in `cvui::init()`, so generally you don't have to watch them
	 yourself. If you initialized cvui and told it *not* to create windows automatically,
	 you need to call `cvui::watch()` on those windows yourself. `cvui::watch()` can
	 automatically create a window before watching it, if it does not exist.
	
	 \param theWindowName name of the window whose UI interactions will be tracked.
	 \param theCreateNamedWindow if an OpenCV window named `theWindowName` should be created before it is watched. Windows are created using `cv::namedWindow()`. If this parameter is `false`, ensure you have called `cv::namedWindow(WINDOW_NAME)` to create the window, otherwise cvui will not be able to track its UI interactions.
	
	 \sa init()
	 \sa context()
	*/
	void watch(const cv::String& theWindowName, bool theCreateNamedWindow = true);

	/**
	 Inform cvui that all subsequent component calls belong to a window in particular.
	 When using cvui with multiple OpenCV windows, you must call cvui component calls
	 between `cvui::contex(NAME)` and `cvui::update(NAME)`, where `NAME` is the name of
	 the window. That way, cvui knows which window you are using (`NAME` in this case),
	 so it can track mouse events, for instance.
	
	 E.g.
	
	 ```
	 // Code for window "window1".
	 cvui::context("window1");
	 cvui::text(frame, ...);
	 cvui::button(frame, ...);
	 cvui::update("window1");
	
	
	 // somewhere else, code for "window2"
	 cvui::context("window2");
	 cvui::printf(frame, ...);
	 cvui::printf(frame, ...);
	 cvui::update("window2");
	
	 // Show everything in a window
	 cv::imshow(frame);
	 ```
	
	 Pay attention to the pair `cvui::context(NAME)` and `cvui::update(NAME)`, which
	 encloses the component calls for that window. You need such pair for each window
	 of your application.
	
	 After calling `cvui::update()`, you can show the result in a window using `cv::imshow()`.
	 If you want to save some typing, you can use `cvui::imshow()`, which calls `cvui::update()`
	 for you and then shows the frame in a window.
	
	 E.g.:
	
	 ```
	 // Code for window "window1".
	 cvui::context("window1");
	 cvui::text(frame, ...);
	 cvui::button(frame, ...);
	 cvui::imshow("window1");
	
	 // somewhere else, code for "window2"
	 cvui::context("window2");
	 cvui::printf(frame, ...);
	 cvui::printf(frame, ...);
	 cvui::imshow("window2");
	 ```
	
	 In that case, you don't have to bother calling `cvui::update()` yourself, since
	 `cvui::imshow()` will do it for you.
	
	 \param theWindowName name of the window that will receive components from all subsequent cvui calls.
	
	 \sa init()
	 \sa watch()
	*/
	void context(const cv::String& theWindowName);

	/**
	 Display an image in the specified window and update the internal structures of cvui.
	 This function can be used as a replacement for `cv::imshow()`. If you want to use
	 `cv::imshow() instead of `cvui::imshow()`, you must ensure you call `cvui::update()`
	 *after* all component calls and *before* `cv::imshow()`, so cvui can update its
	 internal structures.
	
	 In general, it is easier to call `cvui::imshow()` alone instead of calling
	 `cvui::update()' immediately followed by `cv::imshow()`.
	
	 \param theWindowName name of the window that will be shown.
	 \param theFrame image, i.e. `cv::Mat`, to be shown in the window.
	
	 \sa update()
	 \sa context()
	 \sa watch()
	*/
	void imshow(const cv::String& theWindowName, cv::InputArray theFrame);

	/**
	 Return the last key that was pressed. This function will only
	 work if a value greater than zero was passed to `cvui::init()`
	 as the delay waitkey parameter.
	
	 \sa init()
	*/
	int lastKeyPressed();

	/**
	 Return the last position of the mouse.
	
	 \param theWindowName name of the window whose mouse cursor will be used. If nothing is informed (default), the function will return the position of the mouse cursor for the default window (the one informed in `cvui::init()`).
	 \return a point containing the position of the mouse cursor in the speficied window.
	*/
	cv::Point mouse(const cv::String& theWindowName = "");

	/**
	 Query the mouse for events, e.g. "is any button down now?". Available queries are:
	 
	 * `cvui::DOWN`: any mouse button was pressed. `cvui::mouse()` returns `true` for a single frame only.
	 * `cvui::UP`: any mouse button was released.  `cvui::mouse()` returns `true` for a single frame only.
	 * `cvui::CLICK`: any mouse button was clicked (went down then up, no matter the amount of frames in between). `cvui::mouse()` returns `true` for a single frame only.
	 * `cvui::IS_DOWN`: any mouse button is currently pressed. `cvui::mouse()` returns `true` for as long as the button is down/pressed.
	
	 It is easier to think of this function as the answer to a questions. For instance, asking if any mouse button went down:
	
	 ```
	 if (cvui::mouse(cvui::DOWN)) {
	   // Any mouse button just went down.
	 }
	 ```
	
	 The window whose mouse will be queried depends on the context. If `cvui::mouse(query)` is being called after
	 `cvui::context()`, the window informed in the context will be queried. If no context is available, the default
	 window (informed in `cvui::init()`) will be used.
	
	 \param theQuery integer describing the intended mouse query. Available queries are `cvui::DOWN`, `cvui::UP`, `cvui::CLICK`, and `cvui::IS_DOWN`.
	
	 \sa mouse(const cv::String&)
	 \sa mouse(const cv::String&, int)
	 \sa mouse(const cv::String&, int, int)
	 \sa mouse(int, int)
	*/
	bool mouse(int theQuery);

	/**
	 Query the mouse for events in a particular window. This function behave exactly like `cvui::mouse(int theQuery)`
	 with the difference that queries are targeted at a particular window.
	
	 \param theWindowName name of the window that will be queried.
	 \param theQuery integer describing the intended mouse query. Available queries are `cvui::DOWN`, `cvui::UP`, `cvui::CLICK`, and `cvui::IS_DOWN`.
	
	 \sa mouse(const cv::String&)
	 \sa mouse(const cv::String&, int, int)
	 \sa mouse(int, int)
	 \sa mouse(int)
	*/
	bool mouse(const cv::String& theWindowName, int theQuery);

	/**
	 Query the mouse for events in a particular button. This function behave exactly like `cvui::mouse(int theQuery)`,
	 with the difference that queries are targeted at a particular mouse button instead.
	
	 \param theButton integer describing the mouse button to be queried. Possible values are `cvui::LEFT_BUTTON`, `cvui::MIDDLE_BUTTON` and `cvui::LEFT_BUTTON`.
	 \param theQuery integer describing the intended mouse query. Available queries are `cvui::DOWN`, `cvui::UP`, `cvui::CLICK`, and `cvui::IS_DOWN`.
	
	 \sa mouse(const cv::String&)
	 \sa mouse(const cv::String&, int, int)
	 \sa mouse(int)
	*/
	bool mouse(int theButton, int theQuery);

	/**
	 Query the mouse for events in a particular button in a particular window. This function behave exactly
	 like `cvui::mouse(int theButton, int theQuery)`, with the difference that queries are targeted at
	 a particular mouse button in a particular window instead.
	
	 \param theWindowName name of the window that will be queried.
	 \param theButton integer describing the mouse button to be queried. Possible values are `cvui::LEFT_BUTTON`, `cvui::MIDDLE_BUTTON` and `cvui::LEFT_BUTTON`.
	 \param theQuery integer describing the intended mouse query. Available queries are `cvui::DOWN`, `cvui::UP`, `cvui::CLICK`, and `cvui::IS_DOWN`.
	*/
	bool mouse(const cv::String& theWindowName, int theButton, int theQuery);

	/**
	 Display a button. The size of the button will be automatically adjusted to
	 properly house the label content.
	
	 \param theWhere image/frame where the component should be rendered.
	 \param theX position X where the component should be placed.
	 \param theY position Y where the component should be placed.
	 \param theLabel text displayed inside the button.
	 \param theFontScale size of the text.
	 \param theInsideColor the color used to fill the button (other button colors, like its border, are derived from it)
	 \return `true` everytime the user clicks the button.
	*/
	bool button(cv::Mat& theWhere, int theX, int theY, const cv::String& theLabel,
	            double theFontScale = DEFAULT_FONT_SCALE, unsigned int theInsideColor = DEFAULT_BUTTON_COLOR);

	/**
	 Display a button. The button size will be defined by the width and height parameters,
	 no matter the content of the label.
	
	 \param theWhere image/frame where the component should be rendered.
	 \param theX position X where the component should be placed.
	 \param theY position Y where the component should be placed.
	 \param theWidth width of the button.
	 \param theHeight height of the button.
	 \param theLabel text displayed inside the button.
	 \param theFontScale size of the text.
	 \param theInsideColor the color used to fill the button (other button colors, like its border, are derived from it)
	 \return `true` everytime the user clicks the button.
	*/
	bool button(cv::Mat& theWhere, int theX, int theY, int theWidth, int theHeight, const cv::String& theLabel,
	            double theFontScale = DEFAULT_FONT_SCALE, unsigned int theInsideColor = DEFAULT_BUTTON_COLOR);

	/**
	 Display a button whose graphics are images (cv::Mat). The button accepts three images to describe its states,
	 which are idle (no mouse interaction), over (mouse is over the button) and down (mouse clicked the button).
	 The button size will be defined by the width and height of the images. 
	
	 \param theWhere image/frame where the component should be rendered.
	 \param theX position X where the component should be placed.
	 \param theY position Y where the component should be placed.
	 \param theIdle an image that will be rendered when the button is not interacting with the mouse cursor.
	 \param theOver an image that will be rendered when the mouse cursor is over the button.
	 \param theDown an image that will be rendered when the mouse cursor clicked the button (or is clicking).
	 \return `true` everytime the user clicks the button.
	
	 \sa button()
	 \sa image()
	 \sa iarea()
	*/
	bool button(cv::Mat& theWhere, int theX, int theY, cv::Mat& theIdle, cv::Mat& theOver, cv::Mat& theDown);

	/**
	 Display an image (cv::Mat). 
	
	 \param theWhere image/frame where the provded image should be rendered.
	 \param theX position X where the image should be placed.
	 \param theY position Y where the image should be placed.
	 \param theImage image to be rendered in the specified destination.
	
	 \sa button()
	 \sa iarea()
	*/
	void image(cv::Mat& theWhere, int theX, int theY, cv::Mat& theImage);

	/**
	 Display a checkbox. You can use the state parameter to monitor if the
	 checkbox is checked or not.
	
	 \param theWhere image/frame where the component should be rendered.
	 \param theX position X where the component should be placed.
	 \param theY position Y where the component should be placed.
	 \param theLabel text displayed besides the clickable checkbox square.
	 \param theState describes the current state of the checkbox: `true` means the checkbox is checked.
	 \param theColor color of the label in the format `0xRRGGBB`, e.g. `0xff0000` for red.
	 \return a boolean value that indicates the current state of the checkbox, `true` if it is checked.
	*/
	bool checkbox(cv::Mat& theWhere, int theX, int theY, const cv::String& theLabel, bool* theState,
	              unsigned int theColor = 0xCECECE, double theFontScale = DEFAULT_FONT_SCALE);

	/**
	 Display a piece of text.
	
	 \param theWhere image/frame where the component should be rendered.
	 \param theX position X where the component should be placed.
	 \param theY position Y where the component should be placed.
	 \param theText the text content.
	 \param theFontScale size of the text.
	 \param theColor color of the text in the format `0xRRGGBB`, e.g. `0xff0000` for red.
	
	 \sa printf()
	*/
	void text(cv::Mat& theWhere, int theX, int theY, const cv::String& theText,
	          double theFontScale = DEFAULT_FONT_SCALE, unsigned int theColor = 0xCECECE);

	/**
	 Display a piece of text that can be formated using `stdio's printf()` style. For instance
	 if you want to display text mixed with numbers, you can use:
	
	 ```
	 printf(frame, 10, 15, 0.4, 0xff0000, "Text: %d and %f", 7, 3.1415);
	 ```
	
	 \param theWhere image/frame where the component should be rendered.
	 \param theX position X where the component should be placed.
	 \param theY position Y where the component should be placed.
	 \param theFontScale size of the text.
	 \param theColor color of the text in the format `0xRRGGBB`, e.g. `0xff0000` for red.
	 \param theFmt formating string as it would be supplied for `stdio's printf()`, e.g. `"Text: %d and %f", 7, 3.1415`.
	 
	 \sa text()
	*/
	void printf(cv::Mat& theWhere, int theX, int theY, double theFontScale, unsigned int theColor, const char* theFmt,
	            ...);

	/**
	 Display a piece of text that can be formated using `stdio's printf()` style. For instance
	 if you want to display text mixed with numbers, you can use:
	
	 ```
	 printf(frame, 10, 15, 0.4, 0xff0000, "Text: %d and %f", 7, 3.1415);
	 ```
	
	 The size and color of the text will be based on cvui's default values.
	
	 \param theWhere image/frame where the component should be rendered.
	 \param theX position X where the component should be placed.
	 \param theY position Y where the component should be placed.
	 \param theFmt formating string as it would be supplied for `stdio's printf()`, e.g. `"Text: %d and %f", 7, 3.1415`.
	
	 \sa text()
	*/
	void printf(cv::Mat& theWhere, int theX, int theY, const char* theFmt, ...);

	/**
	 Display a counter for integer values that the user can increase/descrease
	 by clicking the up and down arrows.
	
	 \param theWhere image/frame where the component should be rendered.
	 \param theX position X where the component should be placed.
	 \param theY position Y where the component should be placed.
	 \param theValue the current value of the counter.
	 \param theStep the amount that should be increased/decreased when the user interacts with the counter buttons
	 \param theFormat how the value of the counter should be presented, as it was printed by `stdio's printf()`. E.g. `"%d"` means the value will be displayed as an integer, `"%0d"` integer with one leading zero, etc.
	 \param theFontScale size of the text.
	 \param theInsideColor the inside color of the two buttons used for the counter
	 \return integer that corresponds to the current value of the counter, in the format `0xRRGGBB`, e.g. `0xff0000` for red.
	*/
	int counter(cv::Mat& theWhere, int theX, int theY, int* theValue, int theStep = 1, const char* theFormat = "%d",
	            double theFontScale = DEFAULT_FONT_SCALE, unsigned int theInsideColor = DEFAULT_BUTTON_COLOR);

	/**
	 Display a counter for float values that the user can increase/descrease
	 by clicking the up and down arrows.
	
	 \param theWhere image/frame where the component should be rendered.
	 \param theX position X where the component should be placed.
	 \param theY position Y where the component should be placed.
	 \param theValue the current value of the counter.
	 \param theStep the amount that should be increased/decreased when the user interacts with the counter buttons
	 \param theFormat how the value of the counter should be presented, as it was printed by `stdio's printf()`. E.g. `"%f"` means the value will be displayed as a regular float, `"%.2f"` float with two digits after the point, etc.
	 \param theFontScale size of the text.
	 \param theInsideColor the inside color of the two buttons used for the counter
	 \return a float that corresponds to the current value of the counter, in the format `0xRRGGBB`, e.g. `0xff0000` for red.
	*/
	double counter(cv::Mat& theWhere, int theX, int theY, double* theValue, double theStep = 0.5,
	               const char* theFormat = "%.2f", double theFontScale = DEFAULT_FONT_SCALE,
	               unsigned int = DEFAULT_BUTTON_COLOR);

	/**
	 Display a trackbar for numeric values that the user can increase/decrease
	 by clicking and/or dragging the marker right or left. This component uses templates
	 so it is imperative that you make it very explicit the type of `theValue`, `theMin`, `theMax` and `theStep`,
	 otherwise you might end up with weird compilation errors. 
	 
	 Example:
	
	 ```
	 // using double
	 trackbar(where, x, y, width, &doubleValue, 0.0, 50.0);
	
	 // using float
	 trackbar(where, x, y, width, &floatValue, 0.0f, 50.0f);
	
	 // using char
	 trackbar(where, x, y, width, &charValue, (char)1, (char)10);
	 ```
	
	 \param theWhere image/frame where the component should be rendered.
	 \param theX position X where the component should be placed.
	 \param theY position Y where the component should be placed.
	 \param theWidth the width of the trackbar.
	 \param theValue the current value of the trackbar. It will be modified when the user interacts with the trackbar. Any numeric type can be used, e.g. float, double, long double, int, char, uchar.
	 \param theMin minimum value allowed for the trackbar.
	 \param theMax maximum value allowed for the trackbar.
	 \param theSegments number of segments the trackbar will have (default is 1). Segments can be seen as groups of numbers in the scale of the trackbar. For example, 1 segment means a single groups of values (no extra labels along the scale), 2 segments mean the trackbar values will be divided in two groups and a label will be placed at the middle of the scale.
	 \param theLabelFormat formating string that will be used to render the labels, e.g. `%.2Lf` (Lf *not lf). No matter the type of the `theValue` param, internally trackbar stores it as a `long double`, so the formating string will *always* receive a `long double` value to format. If you are using a trackbar with integers values, for instance, you can supress decimals using a formating string such as `%.0Lf` to format your labels.
	 \param theOptions options to customize the behavior/appearance of the trackbar, expressed as a bitset. Available options are defined as `TRACKBAR_` constants and they can be combined using the bitwise `|` operand. Available options are: `TRACKBAR_HIDE_SEGMENT_LABELS` (do not render segment labels, but do render min/max labels), `TRACKBAR_HIDE_STEP_SCALE` (do not render the small lines indicating values in the scale), `TRACKBAR_DISCRETE` (changes of the trackbar value are multiples of theDiscreteStep param), `TRACKBAR_HIDE_MIN_MAX_LABELS` (do not render min/max labels), `TRACKBAR_HIDE_VALUE_LABEL` (do not render the current value of the trackbar below the moving marker), `TRACKBAR_HIDE_LABELS` (do not render labels at all).
	 \param theDiscreteStep amount that the trackbar marker will increase/decrease when the marker is dragged right/left (if option TRACKBAR_DISCRETE is ON)
	 \param theFontScale size of the text.
	 \return `true` when the value of the trackbar changed.
	
	 \sa counter()
	*/
	template <typename T>
	bool trackbar(cv::Mat& theWhere, int theX, int theY, int theWidth, T* theValue, T theMin, T theMax,
	              int theSegments = 1, const char* theLabelFormat = "%.1Lf", unsigned int theOptions = 0,
	              T theDiscreteStep = 1, double theFontScale = DEFAULT_FONT_SCALE);

	/**
	 Display a window (a block with a title and a body).
	
	 \param theWhere image/frame where the component should be rendered.
	 \param theX position X where the component should be placed.
	 \param theY position Y where the component should be placed.
	 \param theWidth width of the window.
	 \param theHeight height of the window.
	 \param theTitle text displayed as the title of the window.
	 \param theFontScale size of the title.
	
	 \sa rect()
	*/
	void window(cv::Mat& theWhere, int theX, int theY, int theWidth, int theHeight, const cv::String& theTitle,
	            double theFontScale = DEFAULT_FONT_SCALE);

	/**
	 Display a filled rectangle.
	
	 \param theWhere image/frame where the component should be rendered.
	 \param theX position X where the component should be placed.
	 \param theY position Y where the component should be placed.
	 \param theWidth width of the rectangle.
	 \param theHeight height of the rectangle.
	 \param theBorderColor color of rectangle's border in the format `0xRRGGBB`, e.g. `0xff0000` for red.
	 \param theFillingColor color of rectangle's filling in the format `0xAARRGGBB`, e.g. `0x00ff0000` for red, `0xff000000` for transparent filling.
	
	 \sa image()
	*/
	void rect(cv::Mat& theWhere, int theX, int theY, int theWidth, int theHeight, unsigned int theBorderColor,
	          unsigned int theFillingColor = 0xff000000);

	/**
	 Display the values of a vector as a sparkline.
	
	 \param theWhere image/frame where the component should be rendered.
	 \param theValues a vector containing the values to be used in the sparkline.
	 \param theX position X where the component should be placed.
	 \param theY position Y where the component should be placed.
	 \param theWidth width of the sparkline.
	 \param theHeight height of the sparkline.
	 \param theColor color of sparkline in the format `0xRRGGBB`, e.g. `0xff0000` for red.
	
	 \sa trackbar()
	*/
	void sparkline(cv::Mat& theWhere, std::vector<double>& theValues, int theX, int theY, int theWidth, int theHeight,
	               unsigned int theColor = 0x00FF00);

	/**
	 Create an interaction area that reports activity with the mouse cursor.
	 The tracked interactions are returned by the function and they are:
	
	 `OUT` when the cursor is not over the iarea.
	 `OVER` when the cursor is over the iarea.
	 `DOWN` when the cursor is pressed over the iarea, but not released yet.
	 `CLICK` when the cursor clicked (pressed and released) within the iarea.
	
	 This function creates no visual output on the screen. It is intended to
	 be used as an auxiliary tool to create interactions.
	
	 \param theX position X where the interactive area should be placed.
	 \param theY position Y where the interactive area should be placed.
	 \param theWidth width of the interactive area.
	 \param theHeight height of the interactive area.
	 \return integer value representing the current state of interaction with the mouse cursor. It can be `OUT` (cursor is not over the area), `OVER` (cursor is over the area), `DOWN` (cursor is pressed over the area, but not released yet) and `CLICK` (cursor clicked, i.e. pressed and released, within the area).
	
	 \sa button()
	 \sa image()
	*/
	int iarea(int theX, int theY, int theWidth, int theHeight);

	/**
	 Start a new row.
	 
	 One of the most annoying tasks when building UI is to calculate 
	 where each component should be placed on the screen. cvui has
	 a set of methods that abstract the process of positioning
	 components, so you don't have to think about assigning a
	 X and Y coordinate. Instead you just add components and cvui
	 will place them as you go.
	
	 You use `beginRow()` to start a group of elements. After `beginRow()`
	 has been called, all subsequent component calls don't have to specify
	 the frame where the component should be rendered nor its position.
	 The position of the component will be automatically calculated by cvui
	 based on the components within the group. All components are placed
	 side by side, from left to right.
	
	 E.g.
	
	 ```
	 beginRow(frame, x, y, width, height);
	  text("test");
	  button("btn");
	 endRow();
	 ```
	
	 Rows and columns can be nested, so you can create columns/rows within
	 columns/rows as much as you want. It's important to notice that any
	 component within `beginRow()` and `endRow()` *do not* specify the position
	 where the component is rendered, which is also true for `beginRow()`.
	 As a consequence, **be sure you are calling `beginRow(width, height)`
	 when the call is nested instead of `beginRow(x, y, width, height)`**,
	 otherwise cvui will throw an error.
	
	 E.g.
	
	 ```
	 beginRow(frame, x, y, width, height);
	  text("test");       
	  button("btn"); 
	
	  beginColumn();      // no frame nor x,y parameters here!
	   text("column1");
	   text("column2");
	  endColumn();
	 endRow();
	 ```
	
	 Don't forget to call `endRow()` to finish the row, otherwise cvui will throw an error.
	
	 \param theWhere image/frame where the components within this block should be rendered.
	 \param theX position X where the row should be placed.
	 \param theY position Y where the row should be placed.
	 \param theWidth width of the row. If a negative value is specified, the width of the row will be automatically calculated based on the content of the block.
	 \param theHeight height of the row. If a negative value is specified, the height of the row will be automatically calculated based on the content of the block.
	 \param thePadding space, in pixels, among the components of the block.
	
	 \sa beginColumn()
	 \sa endRow()
	 \sa endColumn()
	*/
	void beginRow(cv::Mat& theWhere, int theX, int theY, int theWidth = -1, int theHeight = -1, int thePadding = 0);

	/**
	 Ends a row. You must call this function only if you have previously called
	 its counter part, the `beginRow()` function. 
	
	\sa beginRow()
	\sa beginColumn()
	\sa endColumn()
	*/
	void endRow();

	/**
	 Start a new column.
	
	 One of the most annoying tasks when building UI is to calculate
	 where each component should be placed on the screen. cvui has
	 a set of methods that abstract the process of positioning
	 components, so you don't have to think about assigning a
	 X and Y coordinate. Instead you just add components and cvui
	 will place them as you go.
	
	 You use `beginColumn()` to start a group of elements. After `beginColumn()`
	 has been called, all subsequent component calls don't have to specify
	 the frame where the component should be rendered nor its position.
	 The position of the component will be automatically calculated by cvui
	 based on the components within the group. All components are placed
	 below each other, from the top of the screen towards the bottom.
	
	 E.g.
	
	 ```
	 beginColumn(frame, x, y, width, height);
	  text("test");
	  button("btn");
	 endColumn();
	 ```
	
	 Rows and columns can be nested, so you can create columns/rows within
	 columns/rows as much as you want. It's important to notice that any
	 component within `beginColumn()` and `endColumn()` *do not* specify the position
	 where the component is rendered, which is also true for `beginColumn()`.
	 As a consequence, **be sure you are calling `beginColumn(width, height)`
	 when the call is nested instead of `beginColumn(x, y, width, height)`**,
	 otherwise cvui will throw an error.
	
	E.g.
	
	```
	beginColumn(frame, x, y, width, height);
	 text("test");
	 button("btn");
	
	 beginRow();      // no frame nor x,y parameters here!
	  text("column1");
	  text("column2");
	 endRow();
	endColumn();
	```
	
	Don't forget to call `endColumn()` to finish the column, otherwise cvui will throw an error.
	
	\param theWhere image/frame where the components within this block should be rendered.
	\param theX position X where the row should be placed.
	\param theY position Y where the row should be placed.
	\param theWidth width of the column. If a negative value is specified, the width of the column will be automatically calculated based on the content of the block.
	\param theHeight height of the column. If a negative value is specified, the height of the column will be automatically calculated based on the content of the block.
	\param thePadding space, in pixels, among the components of the block.
	
	\sa beginRow()
	\sa endColumn()
	\sa endRow()
	*/
	void beginColumn(cv::Mat& theWhere, int theX, int theY, int theWidth = -1, int theHeight = -1, int thePadding = 0);

	/**
	 End a column. You must call this function only if you have previously called
	 its counter part, i.e. `beginColumn()`.
	
	 \sa beginColumn()
	 \sa beginRow()
	 \sa endRow()
	*/
	void endColumn();

	/**
	Start a row. This function behaves in the same way as `beginRow(frame, x, y, width, height)`,
	however it is suposed to be used within `begin*()/end*()` blocks since they require components
	not to inform frame nor x,y coordinates.
	
	IMPORTANT: this function can only be used within a `begin*()/end*()` block, otherwise it does nothing.
	
	\param theWidth width of the row. If a negative value is specified, the width of the row will be automatically calculated based on the content of the block.
	\param theHeight height of the row. If a negative value is specified, the height of the row will be automatically calculated based on the content of the block.
	\param thePadding space, in pixels, among the components of the block.
	
	\sa beginColumn()
	\sa endRow()
	\sa endColumn()
	*/
	void beginRow(int theWidth = -1, int theHeight = -1, int thePadding = 0);

	/**
	Start a column. This function behaves in the same way as `beginColumn(frame, x, y, width, height)`,
	however it is suposed to be used within `begin*()/end*()` blocks since they require components
	not to inform frame nor x,y coordinates.
	
	IMPORTANT: this function can only be used within a `begin*()/end*()` block, otherwise it does nothing.
	
	\param theWidth width of the column. If a negative value is specified, the width of the column will be automatically calculated based on the content of the block.
	\param theHeight height of the column. If a negative value is specified, the height of the column will be automatically calculated based on the content of the block.
	\param thePadding space, in pixels, among the components of the block.
	
	\sa beginColumn()
	\sa endRow()
	\sa endColumn()
	*/
	void beginColumn(int theWidth = -1, int theHeight = -1, int thePadding = 0);

	/**
	 Add an arbitrary amount of space between components within a `begin*()` and `end*()` block.
	 The function is aware of context, so if it is used within a `beginColumn()` and
	 `endColumn()` block, the space will be vertical. If it is used within a `beginRow()`
	 and `endRow()` block, space will be horizontal.
	
	 IMPORTANT: this function can only be used within a `begin*()/end*()` block, otherwise it does nothing.
	
	 \param theValue the amount of space to be added.
	
	 \sa beginColumn()
	 \sa beginRow()
	 \sa endRow()
	 \sa endColumn()
	*/
	void space(int theValue = 5);

	/**
	 Display a piece of text within a `begin*()` and `end*()` block.
	 
	 IMPORTANT: this function can only be used within a `begin*()/end*()` block, otherwise it does nothing.
	
	 \param theText text content.
	 \param theFontScale size of the text.
	 \param theColor color of the text in the format `0xRRGGBB`, e.g. `0xff0000` for red.
	
	 \sa printf()
	 \sa beginColumn()
	 \sa beginRow()
	 \sa endRow()
	 \sa endColumn()
	*/
	void text(const cv::String& theText, double theFontScale = DEFAULT_FONT_SCALE, unsigned int theColor = 0xCECECE);

	/**
	 Display a button within a `begin*()` and `end*()` block.
	 The button size will be defined by the width and height parameters,
	 no matter the content of the label.
	
	 IMPORTANT: this function can only be used within a `begin*()/end*()` block, otherwise it does nothing.
	
	 \param theWidth width of the button.
	 \param theHeight height of the button.
	 \param theLabel text displayed inside the button. You can set shortcuts by pre-pending them with "&"
	 \param theFontScale size of the text.
	 \param theInsideColor the color used to fill the button (other button colors, like its border, are derived from it) 
	 \return `true` everytime the user clicks the button.
	
	 \sa beginColumn()
	 \sa beginRow()
	 \sa endRow()
	 \sa endColumn()
	*/
	bool button(int theWidth, int theHeight, const cv::String& theLabel, double theFontScale = DEFAULT_FONT_SCALE,
	            unsigned int theInsideColor = DEFAULT_BUTTON_COLOR);

	/**
	 Display a button within a `begin*()` and `end*()` block. The size of the button will be
	 automatically adjusted to properly house the label content.
	
	 IMPORTANT: this function can only be used within a `begin*()/end*()` block, otherwise it does nothing.
	
	 \param theLabel text displayed inside the button. You can set shortcuts by pre-pending them with "&"
	 \param theFontScale size of the text.
	 \param theInsideColor the color used to fill the button (other button colors, like its border, are derived from it)
	 \return `true` everytime the user clicks the button.
	
	 \sa beginColumn()
	 \sa beginRow()
	 \sa endRow()
	 \sa endColumn()
	*/
	bool button(const cv::String& theLabel, double theFontScale = DEFAULT_FONT_SCALE,
	            unsigned int theInsideColor = DEFAULT_BUTTON_COLOR);

	/**
	 Display a button whose graphics are images (cv::Mat).
	
	 IMPORTANT: this function can only be used within a `begin*()/end*()` block, otherwise it does nothing.
	
	 The button accepts three images to describe its states,
	 which are idle (no mouse interaction), over (mouse is over the button) and down (mouse clicked the button).
	 The button size will be defined by the width and height of the images.
	
	 \param theIdle image that will be rendered when the button is not interacting with the mouse cursor.
	 \param theOver image that will be rendered when the mouse cursor is over the button.
	 \param theDown image that will be rendered when the mouse cursor clicked the button (or is clicking).
	 \return `true` everytime the user clicks the button.
	
	 \sa button()
	 \sa image()
	 \sa iarea()
	 \sa beginColumn()
	 \sa beginRow()
	 \sa endRow()
	 \sa endColumn()
	*/
	bool button(cv::Mat& theIdle, cv::Mat& theOver, cv::Mat& theDown);

	/**
	 Display an image (cv::Mat) within a `begin*()` and `end*()` block
	
	 IMPORTANT: this function can only be used within a `begin*()/end*()` block, otherwise it does nothing.
	
	 \param theImage image to be rendered in the specified destination.
	
	 \sa button()
	 \sa iarea()
	 \sa beginColumn()
	 \sa beginRow()
	 \sa endRow()
	 \sa endColumn()
	*/
	void image(cv::Mat& theImage);

	/**
	 Display a checkbox within a `begin*()` and `end*()` block. You can use the state parameter
	 to monitor if the checkbox is checked or not.
	
	 IMPORTANT: this function can only be used within a `begin*()/end*()` block, otherwise it does nothing.
	
	 \param theLabel text displayed besides the clickable checkbox square.
	 \param theState describes the current state of the checkbox: `true` means the checkbox is checked.
	 \param theColor color of the label in the format `0xRRGGBB`, e.g. `0xff0000` for red.
	 \param theFontScale size of the text.
	 \return a boolean value that indicates the current state of the checkbox, `true` if it is checked.
	
	 \sa beginColumn()
	 \sa beginRow()
	 \sa endRow()
	 \sa endColumn()
	*/
	bool checkbox(const cv::String& theLabel, bool* theState, unsigned int theColor = 0xCECECE,
	              double theFontScale = DEFAULT_FONT_SCALE);

	/**
	 Display a piece of text within a `begin*()` and `end*()` block.
	 
	 IMPORTANT: this function can only be used within a `begin*()/end*()` block, otherwise it does nothing.
	
	 The text can be formated using `stdio's printf()` style. For instance if you want to display text mixed
	 with numbers, you can use:
	
	 ```
	 printf(0.4, 0xff0000, "Text: %d and %f", 7, 3.1415);
	 ```
	
	\param theFontScale size of the text.
	\param theColor color of the text in the format `0xRRGGBB`, e.g. `0xff0000` for red.
	\param theFmt formating string as it would be supplied for `stdio's printf()`, e.g. `"Text: %d and %f", 7, 3.1415`.
	
	\sa text()
	\sa beginColumn()
	\sa beginRow()
	\sa endRow()
	\sa endColumn()
	*/
	void printf(double theFontScale, unsigned int theColor, const char* theFmt, ...);

	/**
	 Display a piece of text that can be formated using `stdio's printf()` style.
	 
	 IMPORTANT: this function can only be used within a `begin*()/end*()` block, otherwise it does nothing.
	
	 For instance if you want to display text mixed with numbers, you can use:
	
	 ```
	 printf(frame, 10, 15, 0.4, 0xff0000, "Text: %d and %f", 7, 3.1415);
	 ```
	
	 The size and color of the text will be based on cvui's default values.
	
	 \param theFmt formating string as it would be supplied for `stdio's printf()`, e.g. `"Text: %d and %f", 7, 3.1415`.
	
	 \sa text()
	 \sa beginColumn()
	 \sa beginRow()
	 \sa endRow()
	 \sa endColumn()
	*/
	void printf(const char* theFmt, ...);

	/**
	 Display a counter for integer values that the user can increase/descrease
	 by clicking the up and down arrows.
	
	 IMPORTANT: this function can only be used within a `begin*()/end*()` block, otherwise it does nothing.
	
	 \param theValue the current value of the counter.
	 \param theStep the amount that should be increased/decreased when the user interacts with the counter buttons.
	 \param theFormat how the value of the counter should be presented, as it was printed by `stdio's printf()`. E.g. `"%d"` means the value will be displayed as an integer, `"%0d"` integer with one leading zero, etc.
	 \param theFontScale size of the text
	 \param theInsideColor the inside color of the two buttons used for the counter
	 \return integer that corresponds to the current value of the counter.
	
	\sa printf()
	\sa beginColumn()
	\sa beginRow()
	\sa endRow()
	\sa endColumn()
	*/
	int counter(int* theValue, int theStep = 1, const char* theFormat = "%d", double theFontScale = DEFAULT_FONT_SCALE,
	            unsigned int theInsideColor = DEFAULT_BUTTON_COLOR);

	/**
	 Display a counter for float values that the user can increase/descrease
	 by clicking the up and down arrows.
	
	 IMPORTANT: this function can only be used within a `begin*()/end*()` block, otherwise it does nothing.
	
	 \param theValue the current value of the counter.
	 \param theStep the amount that should be increased/decreased when the user interacts with the counter buttons.
	 \param theFormat how the value of the counter should be presented, as it was printed by `stdio's printf()`. E.g. `"%d"` means the value will be displayed as an integer, `"%0d"` integer with one leading zero, etc.
	 \param theFontScale size of the text.
	 \param theInsideColor the inside color of the two buttons used for the counter
	 \return an float that corresponds to the current value of the counter.
	
	 \sa printf()
	 \sa beginColumn()
	 \sa beginRow()
	 \sa endRow()
	 \sa endColumn()
	*/
	double counter(double* theValue, double theStep = 0.5, const char* theFormat = "%.2f",
	               double theFontScale = DEFAULT_FONT_SCALE, unsigned int theInsideColor = DEFAULT_BUTTON_COLOR);

	/**
	 Display a trackbar for numeric values that the user can increase/decrease
	 by clicking and/or dragging the marker right or left.
	
	 IMPORTANT: this function can only be used within a `begin*()/end*()` block, otherwise it does nothing.
	
	 This component uses templates so it is imperative that you make it very explicit
	 the type of `theValue`, `theMin`, `theMax` and `theStep`, otherwise you might end up with
	 weird compilation errors.
	
	 Example:
	
	 ```
	 // using double
	 trackbar(width, &doubleValue, 0.0, 50.0);
	
	 // using float
	 trackbar(width, &floatValue, 0.0f, 50.0f);
	
	 // using char
	 trackbar(width, &charValue, (char)1, (char)10);
	 ```
	
	 \param theWidth the width of the trackbar.
	 \param theValue the current value of the trackbar. It will be modified when the user interacts with the trackbar. Any numeric type can be used, e.g. float, double, long double, int, char, uchar.
	 \param theMin minimum value allowed for the trackbar.
	 \param theMax maximum value allowed for the trackbar.
	 \param theSegments number of segments the trackbar will have (default is 1). Segments can be seen as groups of numbers in the scale of the trackbar. For example, 1 segment means a single groups of values (no extra labels along the scale), 2 segments mean the trackbar values will be divided in two groups and a label will be placed at the middle of the scale.
	 \param theLabelFormat formating string that will be used to render the labels, e.g. `%.2Lf`. No matter the type of the `theValue` param, internally trackbar stores it as a `long double`, so the formating string will *always* receive a `long double` value to format. If you are using a trackbar with integers values, for instance, you can supress decimals using a formating string as `%.0Lf` to format your labels.
	 \param theOptions options to customize the behavior/appearance of the trackbar, expressed as a bitset. Available options are defined as `TRACKBAR_` constants and they can be combined using the bitwise `|` operand. Available options are: `TRACKBAR_HIDE_SEGMENT_LABELS` (do not render segment labels, but do render min/max labels), `TRACKBAR_HIDE_STEP_SCALE` (do not render the small lines indicating values in the scale), `TRACKBAR_DISCRETE` (changes of the trackbar value are multiples of informed step param), `TRACKBAR_HIDE_MIN_MAX_LABELS` (do not render min/max labels), `TRACKBAR_HIDE_VALUE_LABEL` (do not render the current value of the trackbar below the moving marker), `TRACKBAR_HIDE_LABELS` (do not render labels at all).
	 \param theDiscreteStep the amount that the trackbar marker will increase/decrease when the marker is dragged right/left (if option TRACKBAR_DISCRETE is ON)
	 \param theFontScale size of the text.
	 \return `true` when the value of the trackbar changed.
	
	 \sa counter()
	 \sa beginColumn()
	 \sa beginRow()
	 \sa endRow()
	 \sa endColumn()
	*/
	template <typename T> // T can be any float type (float, double, long double)
	bool trackbar(int theWidth, T* theValue, T theMin, T theMax, int theSegments = 1,
	              const char* theLabelFormat = "%.1Lf", unsigned int theOptions = 0, T theDiscreteStep = 1,
	              double theFontScale = DEFAULT_FONT_SCALE);

	/**
	 Display a window (a block with a title and a body) within a `begin*()` and `end*()` block.
	
	 IMPORTANT: this function can only be used within a `begin*()/end*()` block, otherwise it does nothing.
	
	 \param theWidth width of the window.
	 \param theHeight height of the window.
	 \param theTitle text displayed as the title of the window.
	 \param theFontScale size of the title.
	
	 \sa rect()
	 \sa beginColumn()
	 \sa beginRow()
	 \sa endRow()
	 \sa endColumn()
	*/
	void window(int theWidth, int theHeight, const cv::String& theTitle, double theFontScale = DEFAULT_FONT_SCALE);

	/**
	 Display a rectangle within a `begin*()` and `end*()` block.
	 
	 IMPORTANT: this function can only be used within a `begin*()/end*()` block, otherwise it does nothing.
	
	 \param theWidth width of the rectangle.
	 \param theHeight height of the rectangle.
	 \param theBorderColor color of rectangle's border in the format `0xRRGGBB`, e.g. `0xff0000` for red.
	 \param theFillingColor color of rectangle's filling in the format `0xAARRGGBB`, e.g. `0x00ff0000` for red, `0xff000000` for transparent filling.
	
	 \sa window()
	 \sa beginColumn()
	 \sa beginRow()
	 \sa endRow()
	 \sa endColumn()
	*/
	void rect(int theWidth, int theHeight, unsigned int theBorderColor, unsigned int theFillingColor = 0xff000000);

	/**
	 Display the values of a vector as a sparkline within a `begin*()` and `end*()` block.
	
	 IMPORTANT: this function can only be used within a `begin*()/end*()` block, otherwise it does nothing.
	
	 \param theValues vector with the values that will be rendered as a sparkline.
	 \param theWidth width of the sparkline.
	 \param theHeight height of the sparkline.
	 \param theColor color of sparkline in the format `0xRRGGBB`, e.g. `0xff0000` for red.
	
	 \sa beginColumn()
	 \sa beginRow()
	 \sa endRow()
	 \sa endColumn()
	*/
	void sparkline(std::vector<double>& theValues, int theWidth, int theHeight, unsigned int theColor = 0x00FF00);

	/**
	 Update the library internal things. You need to call this function **AFTER** you are done adding/manipulating
	 UI elements in order for them to react to mouse interactions.
	
	 \param theWindowName name of the window whose components are being updated. If no window name is provided, cvui uses the default window.
	
	 \sa init()
	 \sa watch()
	 \sa context()
	*/
	void update(const cv::String& theWindowName = "");

	// Internally used to handle mouse events
	void handleMouse(int theEvent, int theX, int theY, int theFlags, void* theData);

	// Compatibility macros to allow compilation with either OpenCV 2.x or OpenCV 3.x
#if (CV_MAJOR_VERSION < 3)
	#define CVUI_ANTIALISED CV_AA
#else
#define CVUI_ANTIALISED cv::LINE_AA
#endif
#define CVUI_FILLED -1

	// If we are not in a Windows-based environment, replace Windows-specific functions with
	// their POSIX equivalents.
#if !defined(_MSC_VER)
	#define vsprintf_s vsprintf
	#define sprintf_s sprintf
#endif

	// Adjust things accoridng to platform
#ifdef _MSC_VER
#define _CVUI_COMPILE_MESSAGE(x) message(x)

	// If windows.h has already been included, min() and max() will be defined.
	// In such case, a shit storm will rain on us, producing all kinds of
	// compilation problems with cvui. If min/max are already defined,
	// let's undef them for now and redef them at the end of this file.
#ifdef min
		#define __cvui_min min
		#undef min
#endif
#ifdef max
		#define __cvui_max max
		#undef max
#endif
#elif __GNUC__
	#define _CVUI_COMPILE_MESSAGE(x) message x
#endif

	// Some compilation messages
#define _CVUI_IMPLEMENTATION_NOTICE
#define _CVUI_NO_IMPLEMENTATION_NOTICE

	// Below is paranoic, dramatic bug-fixing action to ensure no evil macro
	// have already defined our names. For some bizarre reason, cvui did not
	// compile in one of my projects because OUT was already defined. So be it.
#undef VERSION
#undef ROW
#undef COLUMN
#undef DOWN
#undef CLICK
#undef OVER
#undef OUT
#undef UP
#undef IS_DOWN
#undef LEFT_BUTTON
#undef MIDDLE_BUTTON
#undef RIGHT_BUTTON

	// Check for Unix stuff
#ifdef __GNUC__
	// just to remove the warning under gcc that is introduced by the VERSION variable below
	// (needed for those who compile with -Werror (make warning as errors)
	#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

	// Lib version
	static const char* VERSION = "2.7.0";

	const int ROW = 0;
	const int COLUMN = 1;
	const int DOWN = 2;
	const int CLICK = 3;
	const int OVER = 4;
	const int OUT = 5;
	const int UP = 6;
	const int IS_DOWN = 7;

	// Constants regarding mouse buttons
	const int LEFT_BUTTON = 0;
	const int MIDDLE_BUTTON = 1;
	const int RIGHT_BUTTON = 2;

	// Constants regarding components
	const unsigned int TRACKBAR_HIDE_SEGMENT_LABELS = 1;
	const unsigned int TRACKBAR_HIDE_STEP_SCALE = 2;
	const unsigned int TRACKBAR_DISCRETE = 4;
	const unsigned int TRACKBAR_HIDE_MIN_MAX_LABELS = 8;
	const unsigned int TRACKBAR_HIDE_VALUE_LABEL = 16;
	const unsigned int TRACKBAR_HIDE_LABELS = 32;

	// Describes the block structure used by the lib to handle `begin*()` and `end*()` calls.
	using cvui_block_t = struct
	{
		cv::Mat where; // where the block should be rendered to.
		cv::Rect rect; // the size and position of the block.
		cv::Rect fill; // the filled area occuppied by the block as it gets modified by its inner components.
		cv::Point anchor; // the point where the next component of the block should be rendered.
		int padding; // padding among components within this block.
		int type; // type of the block, e.g. ROW or COLUMN.
	};

	// Describes a component label, including info about a shortcut.
	// If a label contains "Re&start", then:
	// - hasShortcut will be true
	// - shortcut will be 's'
	// - textBeforeShortcut will be "Re"
	// - textAfterShortcut will be "tart"
	using cvui_label_t = struct
	{
		bool hasShortcut;
		char shortcut;
		std::string textBeforeShortcut;
		std::string textAfterShortcut;
	};

	// Describe a mouse button
	using cvui_mouse_btn_t = struct
	{
		bool justReleased; // if the mouse button was released, i.e. click event.
		bool justPressed; // if the mouse button was just pressed, i.e. true for a frame when a button is down.
		bool pressed; // if the mouse button is pressed or not.
	};

	// Describe the information of the mouse cursor
	using cvui_mouse_t = struct
	{
		cvui_mouse_btn_t buttons[3];
		// status of each button. Use cvui::{RIGHT,LEFT,MIDDLE}_BUTTON to access the buttons.
		cvui_mouse_btn_t anyButton; // represent the behavior of all mouse buttons combined
		cv::Point position; // x and y coordinates of the mouse at the moment.
	};

	// Describes a (window) context.
	using cvui_context_t = struct
	{
		cv::String windowName; // name of the window related to this context.
		cvui_mouse_t mouse; // the mouse cursor related to this context.
	};

	// Internal namespace with all code that is shared among components/functions.
	// You should probably not be using anything from here.
	namespace internal
	{
		static cv::String gDefaultContext;
		static cv::String gCurrentContext;
		static std::map<cv::String, cvui_context_t> gContexts; // indexed by the window name.
		static char gBuffer[1024];
		static int gLastKeyPressed; // TODO: collect it per window
		static int gDelayWaitKey;
		static cvui_block_t gScreen;

		struct TrackbarParams
		{
			long double min;
			long double max;
			long double step;
			int segments;
			unsigned int options;
			std::string labelFormat;
			double fontScale;

			TrackbarParams()
				: min(0.)
				  , max(25.)
				  , step(1.)
				  , segments(0)
				  , options(0)
				  , labelFormat("%.0Lf")
				  , fontScale(DEFAULT_FONT_SCALE)
			{
			}
		};

		static cvui_block_t gStack[100]; // TODO: make it dynamic?
		static int gStackCount = -1;
		static const int gTrackbarMarginX = 14;

		bool isMouseButton(cvui_mouse_btn_t& theButton, int theQuery);
		void resetMouseButton(cvui_mouse_btn_t& theButton);
		void init(const cv::String& theWindowName, int theDelayWaitKey);
		cvui_context_t& getContext(const cv::String& theWindowName = "");
		bool bitsetHas(unsigned int theBitset, unsigned int theValue);
		void error(int theId, std::string theMessage);
		void updateLayoutFlow(cvui_block_t& theBlock, cv::Size theSize);
		bool blockStackEmpty();
		cvui_block_t& topBlock();
		cvui_block_t& pushBlock();
		cvui_block_t& popBlock();
		void begin(int theType, cv::Mat& theWhere, int theX, int theY, int theWidth, int theHeight, int thePadding);
		void end(int theType);
		cvui_label_t createLabel(const std::string& theLabel);
		int iarea(int theX, int theY, int theWidth, int theHeight);
		bool button(cvui_block_t& theBlock, int theX, int theY, int theWidth, int theHeight, const cv::String& theLabel,
		            bool theUpdateLayout, double theFontScale, unsigned int theInsideColor);
		bool button(cvui_block_t& theBlock, int theX, int theY, const cv::String& theLabel, double theFontScale,
		            unsigned int theInsideColor);
		bool button(cvui_block_t& theBlock, int theX, int theY, cv::Mat& theIdle, cv::Mat& theOver, cv::Mat& theDown,
		            bool theUpdateLayout);
		void image(cvui_block_t& theBlock, int theX, int theY, cv::Mat& theImage);
		bool checkbox(cvui_block_t& theBlock, int theX, int theY, const cv::String& theLabel, bool* theState,
		              unsigned int theColor, double theFontScale);
		void text(cvui_block_t& theBlock, int theX, int theY, const cv::String& theText, double theFontScale,
		          unsigned int theColor, bool theUpdateLayout);
		int counter(cvui_block_t& theBlock, int theX, int theY, int* theValue, int theStep, const char* theFormat,
		            double theFontScale);
		double counter(cvui_block_t& theBlock, int theX, int theY, double* theValue, double theStep,
		               const char* theFormat, double theFontScale);
		void window(cvui_block_t& theBlock, int theX, int theY, int theWidth, int theHeight, const cv::String& theTitle,
		            double theFontScale);
		void rect(cvui_block_t& theBlock, int theX, int theY, int theWidth, int theHeight, unsigned int theBorderColor,
		          unsigned int theFillingColor);
		void sparkline(cvui_block_t& theBlock, std::vector<double>& theValues, int theX, int theY, int theWidth,
		               int theHeight, unsigned int theColor);
		bool trackbar(cvui_block_t& theBlock, int theX, int theY, int theWidth, long double* theValue,
		              const TrackbarParams& theParams);
		inline void trackbarForceValuesAsMultiplesOfSmallStep(const TrackbarParams& theParams, long double* theValue);
		inline long double trackbarXPixelToValue(const TrackbarParams& theParams, cv::Rect& theBounding, int thePixelX);
		inline int trackbarValueToXPixel(const TrackbarParams& theParams, cv::Rect& theBounding, long double theValue);
		inline long double clamp01(long double value);
		void findMinMax(std::vector<double>& theValues, double* theMin, double* theMax);
		cv::Scalar hexToScalar(unsigned int theColor);
		unsigned int brightenColor(unsigned int theColor, unsigned int theDelta);
		unsigned int darkenColor(unsigned int theColor, unsigned int theDelta);
		uint8_t brightnessOfColor(unsigned int theColor);
		void resetRenderingBuffer(cvui_block_t& theScreen);

		template <typename T> // T can be any floating point type (float, double, long double)
		TrackbarParams makeTrackbarParams(T min, T max, int theDecimals = 1, int theSegments = 1, T theStep = -1.,
		                                  unsigned int theOptions = 0, const char* theFormat = "%.1Lf",
		                                  double theFontScale = DEFAULT_FONT_SCALE);

		template <typename T>
		bool trackbar(T* theValue, const TrackbarParams& theParams);

		template <typename T> // T can be any numeric type (int, double, unsigned int, etc)
		bool trackbar(cv::Mat& theWhere, int theX, int theY, int theWidth, T* theValue,
		              const TrackbarParams& theParams);

		template <typename num_type>
		TrackbarParams makeTrackbarParams(num_type theMin, num_type theMax, num_type theStep, int theSegments,
		                                  const char* theLabelFormat, unsigned int theOptions, double theFontScale)
		{
			TrackbarParams aParams;

			aParams.min = static_cast<long double>(theMin);
			aParams.max = static_cast<long double>(theMax);
			aParams.step = static_cast<long double>(theStep);
			aParams.options = theOptions;
			aParams.segments = theSegments;
			aParams.labelFormat = theLabelFormat;
			aParams.fontScale = theFontScale;

			return aParams;
		}

		template <typename num_type>
		bool trackbar(int theWidth, num_type* theValue, const TrackbarParams& theParams)
		{
			cvui_block_t& aBlock = topBlock();

			long double aValueAsDouble = static_cast<long double>(*theValue);
			bool aResult = internal::trackbar(aBlock, aBlock.anchor.x, aBlock.anchor.y, theWidth, &aValueAsDouble,
			                                  theParams);
			*theValue = static_cast<num_type>(aValueAsDouble);

			return aResult;
		}

		template <typename num_type>
		bool trackbar(cv::Mat& theWhere, int theX, int theY, int theWidth, num_type* theValue,
		              const TrackbarParams& theParams)
		{
			gScreen.where = theWhere;

			long double aValueAsDouble = static_cast<long double>(*theValue);
			bool aResult = internal::trackbar(gScreen, theX, theY, theWidth, &aValueAsDouble, theParams);
			*theValue = static_cast<num_type>(aValueAsDouble);

			return aResult;
		}
	}

	// Internal namespace that contains all rendering functions.
	namespace render
	{
		void text(cvui_block_t& theBlock, const cv::String& theText, cv::Point& thePos, double theFontScale,
		          unsigned int theColor);
		void button(cvui_block_t& theBlock, int theState, cv::Rect& theShape, double theFontScale,
		            unsigned int theInsideColor);
		void buttonLabel(cvui_block_t& theBlock, int theState, cv::Rect theRect, const cv::String& theLabel,
		                 cv::Size& theTextSize, double theFontScale, unsigned int theInsideColor);
		void image(cvui_block_t& theBlock, cv::Rect& theRect, cv::Mat& theImage);
		void counter(cvui_block_t& theBlock, cv::Rect& theShape, const cv::String& theValue, double theFontScale);
		void trackbarHandle(cvui_block_t& theBlock, int theState, cv::Rect& theShape, double theValue,
		                    const internal::TrackbarParams& theParams, cv::Rect& theWorkingArea);
		void trackbarPath(cvui_block_t& theBlock, int theState, cv::Rect& theWorkingArea);
		void trackbarSteps(cvui_block_t& theBlock, cv::Rect& theShape, const internal::TrackbarParams& theParams,
		                   cv::Rect& theWorkingArea);
		void trackbarSegmentLabel(cvui_block_t& theBlock, cv::Rect& theShape, const internal::TrackbarParams& theParams,
		                          long double theValue, cv::Rect& theWorkingArea, bool theShowLabel);
		void trackbarSegments(cvui_block_t& theBlock, cv::Rect& theShape, const internal::TrackbarParams& theParams,
		                      cv::Rect& theWorkingArea);
		void trackbar(cvui_block_t& theBlock, int theState, cv::Rect& theShape, double theValue,
		              const internal::TrackbarParams& theParams);
		void checkbox(cvui_block_t& theBlock, int theState, cv::Rect& theShape);
		void checkboxLabel(cvui_block_t& theBlock, cv::Rect& theRect, const cv::String& theLabel, cv::Size& theTextSize,
		                   unsigned int theColor, double theFontScale);
		void checkboxCheck(cvui_block_t& theBlock, cv::Rect& theShape);
		void window(cvui_block_t& theBlock, cv::Rect& theTitleBar, cv::Rect& theContent, const cv::String& theTitle,
		            double theFontScale);
		void rect(cvui_block_t& theBlock, cv::Rect& thePos, unsigned int theBorderColor, unsigned int theFillingColor);
		void sparkline(cvui_block_t& theBlock, std::vector<double>& theValues, cv::Rect& theRect, double theMin,
		               double theMax, unsigned int theColor);

		int putText(cvui_block_t& theBlock, int theState, cv::Scalar aColor, const std::string& theText,
		            const cv::Point& thePosition, double theFontScale);
		int putTextCentered(cvui_block_t& theBlock, const cv::Point& position, const std::string& text,
		                    double theFontScale);
	}

	template <typename num_type>
	bool trackbar(cv::Mat& theWhere, int theX, int theY, int theWidth, num_type* theValue, num_type theMin,
	              num_type theMax, int theSegments, const char* theLabelFormat, unsigned int theOptions,
	              num_type theDiscreteStep, double theFontScale)
	{
		internal::TrackbarParams aParams = internal::makeTrackbarParams(theMin, theMax, theDiscreteStep, theSegments,
		                                                                theLabelFormat, theOptions, theFontScale);
		return trackbar<num_type>(theWhere, theX, theY, theWidth, theValue, aParams);
	}

	template <typename num_type>
	bool trackbar(int theWidth, num_type* theValue, num_type theMin, num_type theMax, int theSegments,
	              const char* theLabelFormat, unsigned int theOptions, num_type theDiscreteStep, double theFontScale)
	{
		internal::TrackbarParams aParams = internal::makeTrackbarParams(theMin, theMax, theDiscreteStep, theSegments,
		                                                                theLabelFormat, theOptions, theFontScale);
		return trackbar<num_type>(theWidth, theValue, aParams);
	}
} // namespace cvui

#endif // _CVUI_H_

// Below this line is the implementation of all functions declared above.
