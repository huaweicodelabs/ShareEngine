ShareKitDemo
==============================

The ShareKitDemo app demonstrates how to quickly share files or text on android devices with the capabilities provided by ShareKit

Introduction
------------

- [Read more about Share Kit](https://developer.huawei.com/consumer/cn/doc/development/connectivity-Guides/share-introduction)

Getting Started
---------------

- [Preparation](https://developer.huawei.com/consumer/cn/doc/development/connectivity-Guides/share-preparation-android)
- Run the sample on Android device or emulator.

Sharing Files or Text
---------------------

Text and file sharing using the capabilities provided by Share Kit.

## Quick start
- [Development guides](https://developer.huawei.com/consumer/cn/doc/development/connectivity-Guides/share-guide-android)

## Use ShareKit capabilities
Three-party applications use ShareKitManager class to complete the call to ShareKit capabilities.

- [ShareKit capabilities](https://developer.huawei.com/consumer/cn/doc/development/connectivity-References/share-api-android-sharekitmanager)

## ShareKit result code
ShareKit operation result code definition

- [Result code](https://developer.huawei.com/consumer/cn/doc/development/connectivity-References/share-api-android-kitresult)


## Initial callback
When the ShareKit service is init completed, the application is notified through the callback method.

- [Initial callback](https://developer.huawei.com/consumer/cn/doc/development/connectivity-References/share-api-android-ishareKitInitcallback)

## Process callback
After registering to ShareKit, the corresponding process event will notify through the callback.

- [Process callback](https://developer.huawei.com/consumer/cn/doc/development/connectivity-References/share-api-android-iwidgetcallback)

## File receive finish broadcast
ShareKit receive complete will send a local broadcast to notify application

- [Finished broadcast](https://developer.huawei.com/consumer/cn/doc/development/connectivity-References/share-api-android-broadcast)


## ShareKit device identification
The ShareKit Device identification instances are obtained through ShareKit methods and callbacks

- [Device identification](https://developer.huawei.com/consumer/cn/doc/development/connectivity-References/share-api-android-nearbydeviceex)

## ShareKit message bean
The sharing information encapsulation class

- [Share bean](https://developer.huawei.com/consumer/cn/doc/development/connectivity-References/share-api-android-sharebean)


Result
-----------
<img src="app/src/screen.png" height="534" width="300"/>


License
-------

Copyright 2016 Huawei.

Licensed to the Apache Software Foundation (ASF) under one or more contributor
license agreements.  See the NOTICE file distributed with this work for
additional information regarding copyright ownership.  The ASF licenses this
file to you under the Apache License, Version 2.0 (the "License"); you may not
use this file except in compliance with the License.  You may obtain a copy of
the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
License for the specific language governing permissions and limitations under
the License.
