/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: AllConnect Platform ShareEngine API
 * Create: 2019-10-10
 */

#ifndef SHAREKIT_API_H
#define SHAREKIT_API_H

#include "sharekit_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * @function   : InitShareKit
 * @brief      : initialize the share SDK
 * @param[IN]  : handle,Initialization parameters of the share SDK
 * @param[OUT] : N/A
 * @return     : 0-sucess,other value-failed
 */
int InitShareKit(void* handle);

/*
 * @function   : UninitShareKit
 * @brief      : uninitialized the share SDK
 * @return     : 0-sucess,other value-failed
 */
int UninitShareKit(void);

/*
 * @function   : StartShareService
 * @brief      : Start the share service according to role
 * @param[IN]  : role, Service role like transimitter or receiver
 * @param[IN]  : handle,Startup parameters of the share SDK
 * @param[OUT] : N/A
 * @return     : 0-sucess,other value-failed
 */
int StartShareService(ShareRole role, void* handle);

/*
 * @function   : StopShareService
 * @brief      : Stop the share service
 * @return     : 0-sucess,other value-failed
 */
int StopShareService(void);

/*
 * @function   : GetAvailableDevice
 * @brief      : Get available device
 * @param[IN]  : N/A
 * @param[OUT] : devInfo, Details of the available devices
 * @param[OUT] : devNum, Number of the available devices
 * @return     : 0-sucess,other value-failed
 */
int GetAvailableDevice(ShareDeviceInfo** devInfo, int* devNum);

/*
 * @function   : TransFile
 * @brief      : transfer files
 * @param[IN]  : device, Information of the shared device
 * @param[IN]  : files, Information of the shared file
 * @param[OUT] : N/A
 * @return     : 0-sucess,other value-failed
 */
int TransFile(const ShareDeviceInfo* device, const ShareFileInfo* files);

/*
 * @function   : CancelTransFile
 * @brief      : Cancel the sending of files
 * @return     : void
 */
void CancelTransFile(void);

/*
 * @function   : ConfirmRecvFile
 * @brief      : Confirm to receive the file
 * @param[IN]  : path, The path where the file to be saved
 * @param[OUT] : N/A
 * @return     : 0-sucess,other value-failed
 */
int ConfirmRecvFile(const char* path);

/*
 * @function : RefuseRecvFile
 * @brief    : Refuse to receive the file
 * @return   : void
 */
void RefuseRecvFile(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SHAREKIT_API_H */