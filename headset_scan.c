/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2005-2008
*/

/*!
@file    headset_scan.c
@brief   Implementation of functions controlling discoverable and connectable status 
*/


/****************************************************************************
    Header files
*/

#include "headset_debug.h"
#include "headset_scan.h"
#include "panic.h"
#include "string.h"
#include "stdlib.h"

#include <connection.h>


#ifdef DEBUG_SCAN
#define SCAN_DEBUG(x) DEBUG(x)
#else
#define SCAN_DEBUG(x) 
#endif


#define HCI_PAGESCAN_INTERVAL_DEFAULT  (0x800)
#define HCI_PAGESCAN_WINDOW_DEFAULT   (0x12)
#define HCI_INQUIRYSCAN_INTERVAL_DEFAULT  (0x800)
#define HCI_INQUIRYSCAN_WINDOW_DEFAULT   (0x12)

/****************************************************************************
    Definitions used in EIR data setup
*/
/* Packet size defines */
#define MAX_PACKET_SIZE_DH1 (27)
#define EIR_MAX_SIZE        (MAX_PACKET_SIZE_DH1)

/* EIR tags */
#define EIR_TYPE_LOCAL_NAME_COMPLETE        (0x09)
#define EIR_TYPE_LOCAL_NAME_SHORTENED       (0x08)
#define EIR_TYPE_UUID16_COMPLETE            (0x03)

/****************************************************************************
NAME    
    headsetWriteEirData
    
DESCRIPTION
    Writes the local name and device UUIDs into device EIR data, local name 
        is shortened to fit into a DH1 packet if necessary

RETURNS
    void
*/
void headsetWriteEirData(hsTaskData *app, CL_DM_LOCAL_NAME_COMPLETE_T *message)
{
    const uint8 eir_uuids[] = { EIR_TYPE_UUID16_COMPLETE, /* Complete list of 16-bit Service Class UUIDs */
                                0x1E, 0x11,     /* HFP 0x111E */
                                0x08, 0x11,     /* HSP 0x1108 */
                                0x0B, 0x11,     /* AudioSink 0x110B */
                                0x0D, 0x11,     /* A2DP 0x110D */
                                0x0E, 0x11 };   /* AVRCP 0x11OE */

    /* Length of EIR data with all fields complete */
#define EIR_DATA_SIZE_FULL (message->size_local_name + 2 + sizeof(eir_uuids) + 1 + 1)
    
    /* Whether the EIR data is shortened or not. */
#define EIR_DATA_SHORTENED (EIR_DATA_SIZE_FULL > EIR_MAX_SIZE)

    /* Maximum length the local name can be to fit EIR data into DH1 */
#define EIR_NAME_MAX_SIZE (EIR_MAX_SIZE - (2 + sizeof(eir_uuids) + 1 + 1))

    /* Actual length of the local name put into the EIR data */
#define EIR_NAME_SIZE (EIR_DATA_SHORTENED ? EIR_NAME_MAX_SIZE : message->size_local_name)

    /* Determine length of EIR data */ 
    uint16 size = EIR_DATA_SHORTENED ? EIR_MAX_SIZE : EIR_DATA_SIZE_FULL;

    /* Just enough for the UUID16 and name fields and null termination */
    uint8 *const eir = (uint8 *)PanicUnlessMalloc(size * sizeof(uint8));
    uint8 *p = eir;

    *p++ = EIR_NAME_SIZE + 1;
    *p++ = EIR_DATA_SHORTENED ? EIR_TYPE_LOCAL_NAME_SHORTENED : EIR_TYPE_LOCAL_NAME_COMPLETE;
    memcpy(p, message->local_name, EIR_NAME_SIZE);
    p += EIR_NAME_SIZE;

    *p++ = sizeof(eir_uuids); /* UUIDs length field */
    memcpy(p, eir_uuids, sizeof(eir_uuids));
    p += sizeof(eir_uuids);
    *p++ = 0x00; /* Termination. p ends up pointing one off the end */

    ConnectionWriteEirData(FALSE, size, eir);

    /* Free the EIR data */
    free(eir);
}


/*****************************************************************************/
void headsetEnableConnectable(hsTaskData *app)
{
    hci_scan_enable scan = hci_scan_enable_off;

    /* Set the page scan params */
    /* TODO - Read radio from PSKeys */
    /*ConnectionWritePagescanActivity(app->radio.page_scan_interval, app->radio.page_scan_window);*/
    /*ConnectionWritePagescanActivity(HCI_PAGESCAN_INTERVAL_DEFAULT, HCI_PAGESCAN_WINDOW_DEFAULT);*/

    /* Make sure that if we're inquiry scanning we don't disable it */
    if (app->inquiry_scan_enabled)
        scan = hci_scan_enable_inq_and_page;
    else
        scan = hci_scan_enable_page;

    /* Enable scan mode */
    ConnectionWriteScanEnable(scan);
    
    SCAN_DEBUG(("Scan : %x\n",scan));

    /* Set the flag to indicate we're page scanning */
    app->page_scan_enabled = TRUE;
}


/*****************************************************************************/
void headsetDisableConnectable(hsTaskData *app)
{
    hci_scan_enable scan;

    /* Make sure that if we're inquiry scanning we don't disable it */
    if (app->inquiry_scan_enabled)
        scan = hci_scan_enable_inq;
    else
        scan = hci_scan_enable_off;

    /* Enable scan mode */
    ConnectionWriteScanEnable(scan);
    
    SCAN_DEBUG(("Scan : %x\n",scan));

    /* Set the flag to indicate we're page scanning */
    app->page_scan_enabled = FALSE;
}


/*****************************************************************************/
void headsetEnableDiscoverable(hsTaskData *app)
{
    hci_scan_enable scan = hci_scan_enable_off;

    /* Set the inquiry scan params */
    /* TODO - Read radio from PSKeys */
    /*ConnectionWriteInquiryscanActivity(app->radio.inquiry_scan_interval, app->radio.inquiry_scan_window);*/
    /*ConnectionWriteInquiryscanActivity(HCI_INQUIRYSCAN_INTERVAL_DEFAULT, HCI_INQUIRYSCAN_WINDOW_DEFAULT);*/

    /* Make sure that if we're page scanning we don't disable it */
    if (app->page_scan_enabled)
        scan = hci_scan_enable_inq_and_page;
    else
        scan = hci_scan_enable_inq;

    /* Enable scan mode */
    ConnectionWriteScanEnable(scan);
    
    SCAN_DEBUG(("Scan : %x\n",scan));

    /* Set the flag to indicate we're page scanning */
    app->inquiry_scan_enabled = TRUE;
}


/*****************************************************************************/
void headsetDisableDiscoverable(hsTaskData *app)
{
    hci_scan_enable scan;
    
    /* Make sure that if we're page scanning we don't disable it */
    if (app->page_scan_enabled)
        scan = hci_scan_enable_page;
    else
        scan = hci_scan_enable_off;

    /* Enable scan mode */
    ConnectionWriteScanEnable(scan);
    
    SCAN_DEBUG(("Scan : %x\n",scan));

    /* Set the flag to indicate we're page scanning */
    app->inquiry_scan_enabled = FALSE;
}
