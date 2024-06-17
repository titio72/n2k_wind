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

	static void setdebug();
};


#endif /* LOG_H_ */