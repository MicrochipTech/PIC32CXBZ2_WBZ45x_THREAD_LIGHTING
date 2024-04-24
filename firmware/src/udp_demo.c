/*******************************************************************************
 System Tasks File

  File Name:
    udp_demo.c

  Summary:
    This file contains source code necessary to send udp data using openthread.

  Description:
    This file contains source code necessary to send udp data using openthread.
    
  Remarks:
    
 *******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2023 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *******************************************************************************/
// DOM-IGNORE-END

#ifdef UDP_DEMO
// *****************************************************************************
// *****************************************************************************
// Section: Included Files




// *****************************************************************************
// *****************************************************************************


void threadReceiveData(const otMessageInfo *aMessageInfo, uint16_t length, uint8_t *msgPayload)
{
    char string[OT_IP6_ADDRESS_STRING_SIZE];
    otIp6AddressToString(&(aMessageInfo->mPeerAddr), string, OT_IP6_ADDRESS_STRING_SIZE);
     
    
    USER_LED_Off();
    if( xTimerIsTimerActive( Data_receive_LED_Timer_Handle ) != pdFALSE )
    {
        /* xTimer is active, do something. */
        (void)xTimerStop( Data_receive_LED_Timer_Handle, pdMS_TO_TICKS(0) );
    }
    (void)xTimerStart(Data_receive_LED_Timer_Handle,pdMS_TO_TICKS(0));
    
    devMsgType_t *rxMsg;
    rxMsg = (devMsgType_t *)msgPayload;
    
//    app_printf("App_Log: UDP Received from [%s] len:[%d] type:[%d]\r\n", string, length, rxMsg->msgType);     
    
    
    if(MSG_TYPE_GATEWAY_DISCOVER_REQ == rxMsg->msgType)
    {
        memcpy(&gatewayAddr, rxMsg->msg, OT_IP6_ADDRESS_SIZE);
        demoMsg.msgType = MSG_TYPE_GATEWAY_DISCOVER_RESP;
        memcpy(&demoMsg.msg, &threadDevice, sizeof(devDetails_t));
        threadUdpSend(&gatewayAddr, sizeof(devMsgType_t), (uint8_t *)&demoMsg);
//        app_printf("App Log: DiscReq\r\n");
    }
    else if(MSG_TYPE_LIGHT_SET == rxMsg->msgType)
    {
        app_printf("Rx - %s\n", rxMsg->msg);
        devTypeRGBLight_t *setRGBValue = (devTypeRGBLight_t *)rxMsg->msg;
        rgbLED.onOff = setRGBValue->onOff;
        rgbLED.hue = setRGBValue->hue;
        rgbLED.saturation = setRGBValue->saturation;
        rgbLED.level = setRGBValue->level;
        if( xTimerIsTimerActive(LED_Timer_Handle ) != pdFALSE )
        {
            /* xTimer is active, do something. */
            (void)xTimerStop( LED_Timer_Handle, pdMS_TO_TICKS(0) );
        }
        if(rgbLED.onOff)
        {
            RGB_LED_SetLedColorHSV(rgbLED.hue, rgbLED.saturation, rgbLED.level);
        }
        else
        {
            if(255 == rgbLED.level && 255 == rgbLED.hue && 255 == rgbLED.saturation)
            {
                RGB_LED_SetLedColorHSV(0,255,255);
                (void)xTimerStart(LED_Timer_Handle,pdMS_TO_TICKS(0));
            }
            else
            {
                RGB_LED_SetLedColorHSV(0,0,0);
            }
        }

        demoMsg.msgType = MSG_TYPE_LIGHT_REPORT;

        memcpy(&demoMsg.msg, &rgbLED, sizeof(devTypeRGBLight_t));
        threadUdpSend(&gatewayAddr, sizeof(devMsgType_t), (uint8_t *)&demoMsg);
    }
    else if(MSG_TYPE_LIGHT_GET == rxMsg->msgType)
    {
        demoMsg.msgType = MSG_TYPE_LIGHT_REPORT;

        memcpy(&demoMsg.msg, &rgbLED, sizeof(devTypeRGBLight_t));
        threadUdpSend(&gatewayAddr, sizeof(devMsgType_t), (uint8_t *)&demoMsg);
    }
}

void otUdpReceiveCb(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    APP_Msg_T appMsg_otUdpReceiveCb;
    memset(&appMsg_otUdpReceiveCb, 0, sizeof(APP_Msg_T));
    appMsg_otUdpReceiveCb.msgId = APP_MSG_OT_RECV_CB;
    
    uint16_t aMessageLen = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);
    uint8_t aMessageInfoLen = sizeof(otMessageInfo);
    
    appMsg_otUdpReceiveCb.msgData[0] = aMessageInfoLen;
    memcpy(&appMsg_otUdpReceiveCb.msgData[MESSAGE_INFO_INDEX], aMessageInfo, sizeof(otMessageInfo));

    appMsg_otUdpReceiveCb.msgData[aMessageInfoLen + MESSAGE_INFO_INDEX + 1] = (uint8_t)aMessageLen;

    otMessageRead(aMessage,otMessageGetOffset(aMessage), &appMsg_otUdpReceiveCb.msgData[aMessageInfoLen + MESSAGE_INFO_INDEX + 2], aMessageLen);
    
//    app_printf("App_Log: Data Received\r\n");
    OSAL_QUEUE_SendISR(&appData.appQueue, &appMsg_otUdpReceiveCb);
//    OSAL_QUEUE_Send(&appData.appQueue, &appMsg_otUdpReceiveCb, 0);
}

//void otUdpReceiveCb(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
//{
//    APP_Msg_T appMsg_otUdpReceiveCb;
//    
//    uint16_t len = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);
//    uint8_t output_buffer[len+1];
//    
//    otMessageRead(aMessage,otMessageGetOffset(aMessage),output_buffer,len);
//    output_buffer[len] = '\0';
//    
//    
//    appMsg_otUdpReceiveCb.msgId = APP_MSG_OT_RECV_CB;
//    
//    otUdpReceiveData_t *otUdpReceiveData;
//    otUdpReceiveData = (otUdpReceiveData_t *)&appMsg_otUdpReceiveCb;
//        
//    memcpy(otUdpReceiveData->messageInfo, aMessageInfo, sizeof(otMessageInfo));
//    otUdpReceiveData->length = (uint8_t)len;
//    memcpy(otUdpReceiveData->msgPayload, &output_buffer, len);
//
//    OSAL_QUEUE_Send(&appData.appQueue, &appMsg_otUdpReceiveCb, 0);
//    
//}

void threadUdpOpen()
{
   otError err;
   app_printf("App_log: UDP Open\n");
   err = otUdpOpen(instance, &aSocket, otUdpReceiveCb, NULL);
   if (err != OT_ERROR_NONE)
   {
      app_printf("App_Err: UDP Open failed\n");
       //print error code
       assert(err);
   }
    /* The timer created for LED that blinks when it receives the data from the Leader */
    Data_sent_LED_Timer_Handle = xTimerCreate("Milli_Timer",pdMS_TO_TICKS(LED_BLINK_TIME_MS),pdFALSE, ( void * ) 0, Data_sent_LED_Timer_Callback);
    /* The timer created for LED that blinks when it sends data to the Leader*/
    Data_receive_LED_Timer_Handle = xTimerCreate("Milli_Timer",pdMS_TO_TICKS(LED_BLINK_TIME_MS),pdFALSE, ( void * ) 0, Data_receive_LED_Timer_Callback);
    
    LED_Timer_Handle = xTimerCreate("Milli_Timer",pdMS_TO_TICKS(LED_TIME_MS),true, ( void * ) 0, LED_Timer_Callback);
}

void threadUdpSend(otIp6Address *mPeerAddr, uint8_t msgLen, uint8_t* msg)
{
    otError err = OT_ERROR_NONE;
    otMessageInfo msgInfo;
//    const otIp6Address *mPeerAddr;
    const otIp6Address *mSockAddr;
    memset(&msgInfo,0,sizeof(msgInfo));
//    otIp6AddressFromString("ff03::1",&msgInfo.mPeerAddr);
    mSockAddr = otThreadGetMeshLocalEid(instance);
//    mPeerAddr = otThreadGetRealmLocalAllThreadNodesMulticastAddress(instance);
    memcpy(&msgInfo.mSockAddr, mSockAddr, OT_IP6_ADDRESS_SIZE);
    memcpy(&msgInfo.mPeerAddr, mPeerAddr, OT_IP6_ADDRESS_SIZE);
    
    msgInfo.mPeerPort = UDP_PORT_NO;
    
    do {
        otMessage *udp_msg = otUdpNewMessage(instance,NULL);
        err = otMessageAppend(udp_msg,msg,msgLen);
        if(err != OT_ERROR_NONE)
        {
            app_printf("App_Err: UDP Message Add fail\n");
            break;
        }
        
        err = otUdpSend(instance,&aSocket,udp_msg,&msgInfo);
        if(err != OT_ERROR_NONE)
        {
            app_printf("App_Err: UDP Send fail\n");
            break;
        }
        app_printf("App_Log: UDP Sent data: %d\r\n",err);
//        RGB_LED_GREEN_On();
        if( xTimerIsTimerActive( Data_sent_LED_Timer_Handle ) != pdFALSE )
        {
            /* xTimer is active, do something. */
            (void)xTimerStop( Data_sent_LED_Timer_Handle, pdMS_TO_TICKS(0) );
        }
        (void)xTimerStart(Data_sent_LED_Timer_Handle,pdMS_TO_TICKS(0));

    }while(false);
    
}

//void threadUdpSendAddress(otIp6Address mPeerAddr)
//{
//    otError err = OT_ERROR_NONE;
//    otMessageInfo msgInfo;
//    memset(&msgInfo,0,sizeof(msgInfo));
//    memcpy(&msgInfo.mPeerAddr, &mPeerAddr, OT_IP6_ADDRESS_SIZE);
//    msgInfo.mPeerPort = UDP_PORT_NO;
//    
//    do {
//        otMessage *udp_msg = otUdpNewMessage(instance,NULL);
//        err = otMessageAppend(udp_msg,msg,(uint16_t)strlen(msg));
//        if(err != OT_ERROR_NONE)
//        {
//            app_printf("App_Err: UDP Message Add fail\n");
//            break;
//        }
//        
//        err = otUdpSend(instance,&aSocket,udp_msg,&msgInfo);
//        if(err != OT_ERROR_NONE)
//        {
//            app_printf("App_Err: UDP Send fail\n");
//            break;
//        }
//        app_printf("App_Log: UDP Sent data: %s\n",msg);
//        RGB_LED_GREEN_On();
//        if( xTimerIsTimerActive( Data_sent_LED_Timer_Handle ) != pdFALSE )
//        {
//            /* xTimer is active, do something. */
//            (void)xTimerStop( Data_sent_LED_Timer_Handle, pdMS_TO_TICKS(0) );
//        }
//        (void)xTimerStart(Data_sent_LED_Timer_Handle,pdMS_TO_TICKS(0));
//        
//    }while(false);
//}

void threadUdpBind()
{
   otError err;
   otSockAddr addr;
   memset(&addr,0,sizeof(otSockAddr));
   addr.mPort = UDP_PORT_NO;
   do
   {
        err = otUdpBind(instance, &aSocket, &addr, OT_NETIF_THREAD);
        if (err != OT_ERROR_NONE) {
            app_printf("App_Err: UDP Bind fail Err:%d\n",err);
            break;
        }
        app_printf("App_Log: UDP Listening on port %d\n",UDP_PORT_NO);
   }while(false);
}

//void threadUdpReceiveCb(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
//{
//    uint16_t len = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);
//    uint8_t output_buffer[len+1];
//    char string[OT_IP6_ADDRESS_STRING_SIZE];
//    otIp6AddressToString(&(aMessageInfo->mPeerAddr), string, OT_IP6_ADDRESS_STRING_SIZE);
//    
//    otMessageRead(aMessage,otMessageGetOffset(aMessage),output_buffer,len);
//    output_buffer[len] = '\0';
//    app_printf("App_Log: UDP Received from %s data: %s\n", string, output_buffer);
//    threadReceiveData((otIp6Address *)&(aMessageInfo->mPeerAddr), len, output_buffer);
//    USER_LED_Off();
//    if( xTimerIsTimerActive( Data_receive_LED_Timer_Handle ) != pdFALSE )
//    {
//        /* xTimer is active, do something. */
//        (void)xTimerStop( Data_receive_LED_Timer_Handle, pdMS_TO_TICKS(0) );
//    }
//    (void)xTimerStart(Data_receive_LED_Timer_Handle,pdMS_TO_TICKS(0));
//    
//#if (DEVICE_AS_LEADER==0)
////    threadUdpSendAddress(aMessageInfo->mPeerAddr);
//#endif
//}


#endif
/*******************************************************************************
 End of File
 */

