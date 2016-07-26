// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file gateway.h
*	@brief Extends the Gateway_LL library with additional features.
*
*	@details Gateway extends the Gateway_LL library with 1 additional
*		feature:
*			- Creating a gateway from a JSON configuration file.
*/

#ifndef GATEWAY_H
#define GATEWAY_H

#include "gateway_ll.h"

#ifdef __cplusplus
extern "C"
{
#endif

	/**
	* @brief	Creates a Gateway using a JSON configuration file as input which 
	*			describes each module.
	*
	* @param	file_path	Path to the JSON configuration file for this gateway.
	*			Sample JSON configuration file:
	*
	*				{
	*					"modules" :
	*					[ 
	*						{
	*							"module name" : "foo",
	*							"module path" : "F:\\foo.dll",
	*							"args" : ...
	*						},
	*						{
	*							"module name" : "bar",
	*							"module path" : "F:\\bar.dll",
	*							"args" : ...
	*						}
	*					],
	*					"links":
    *					[
	*						{
	*							"source": "foo",
	*							"sink": "bar"
	*						}
	*					]
	*				}
	*
	* @return	A non-NULL #GATEWAY_HANDLE that can be used to manage the 
	*			gateway or @c NULL on failure.
	*/
	extern GATEWAY_HANDLE Gateway_Create_From_JSON(const char* file_path);

#ifdef __cplusplus
}
#endif

#endif // GATEWAY_H
