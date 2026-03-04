/*
 * MMFRC522.h
 *
 *  Created on: Feb 19, 2026
 *      Author: nxf86731
 */
typedef enum
{
    /* Generic error codes */
    STATUS_SUCCESS                         = 0x000U,    /*!< Generic operation success status */
    STATUS_ERROR                           = 0x001U,    /*!< Generic operation failure status */
    STATUS_BUSY                            = 0x002U,    /*!< Generic operation busy status */
    STATUS_TIMEOUT                         = 0x003U,    /*!< Generic operation timeout status */
    STATUS_UNSUPPORTED                     = 0x004U,    /*!< Generic operation unsupported status */
} status_t;