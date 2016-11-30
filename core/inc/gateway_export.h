// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file       gateway_export.h
 *   @brief     Defines the appropriate function export.
 *
 *   @details   Architecture dependent function declarations.
 */

#ifndef GATEWAY_EXPORT_H
#define GATEWAY_EXPORT_H

#ifdef _WIN32
#define GATEWAY_EXPORT __declspec(dllexport)
#else
#define GATEWAY_EXPORT extern
#endif // _WIN32

#endif // GATEWAY_EXPORT_H
