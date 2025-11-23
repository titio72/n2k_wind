/*
 * Log.h
 *
 *  Created on: Mar 26, 2019
 *      Author: aboni
 */

#ifndef LOG_H_
#define LOG_H_

#define LOG()



class Log {
public:
	static void debug(const char* text, ...);
	static void trace(const char* text, ...);
	static void debugx(const char* module, const char* action, const char* text, ...);
	static void tracex(const char* module, const char* action, const char* text, ...);
	static void tracex(const char* module, const char* action);

	static void setdebug();

	static void enable();
	static void disable();

	static bool is_enabled();
};

#endif /* LOG_H_ */
