//******************************************************************************
//  TMS470R1A256 Demo - 9,600 Baud UART using HET, Polling
//
//  Description: This program implements a 9,600 Baud UART using the HET. The
//  received character is echo'ed back. Polling is used.
//  Pin HET24 is used for TX, HET31 for RX. Note that any HET pins can be used.
//  SYSCLK = 4x12MHz = 48MHz.
//
//            TMS470R1A256
//          +---------------+
//          |               |
//      +---|PLLDIS         |
//      |   |               |
//     ---  |               |
//     GND  |               |
//          |       XIN/XOUT|<--- 12MHz crystal
//          |               |
//          |          HET24|---> 9,600 Baud UART
//          |          HET31|<---
//          |               |
//          +---------------+
//
//  Andreas Dannenberg
//  Texas Instruments, Inc
//  June 06th 2006
//  Built with IAR Embedded Workbench Version: 4.31A
//  A256 EVM
//******************************************************************************

// Include Standard C Language Header Files
#include <stdlib.h>
#include <stdio.h>

// Include TMS470 Specific Header Files
#include <intrinsic.h>
#include "iotms470r1a256.h"
#include "tms470r1a256_bit_definitions.h"
#include "std_het.h"
#include "A256_HET_UART_9600_H.h"

__no_init volatile HETPROGRAM0_UN e_HETPROGRAM0_UN @ 0x00800000;

//------------------------------------------------------------------------------
// Help function to load the HET program into the HET RAM using 32-bit access.
//------------------------------------------------------------------------------
void MemCopy32(unsigned long *dst, unsigned long *src, int bytes)
{
  for (int i = 0; i < (bytes + 3) / 4; i++)
    *dst++ = *src++;
}
//------------------------------------------------------------------------------
// void HetUARTPutByte(unsigned char Data)
//
// This function initializes the transmit one byte using the HET UART.
// The transmit is performed ARM7 independent in the background by the
// HET program.
//------------------------------------------------------------------------------
void HetUARTPutByte(unsigned char Data)
{
  unsigned int Tmp = Data;

  Tmp <<= 1;                                    // Shift in start bit (0)
  Tmp |= 0x00000200;                            // Add stop bit (1)

  HET_TX_Shift_0.memory.data_word = Tmp << 5;   // Load TX buffer
  HET_TX_BitCtr_0.memory.data_word = 10 << 5;   // Load bit count
  HET_TX_Start_0.memory.data_word = 0x1 << 5;   // Start TX
}
//------------------------------------------------------------------------------
// unsigned char HetUARTGetByte(void)
//
// This function reads out the last received character from the HET UART.
//------------------------------------------------------------------------------
unsigned char HetUARTGetByte(void)
{
  unsigned int Tmp = HET_RX_Buffer_0.memory.data_word >> 16;

  HET_RX_Buffer_0.memory.data_word = 0;         // Clear RX buffer
  return Tmp;                                   // Return received byte
}
//------------------------------------------------------------------------------
// char HetUARTRxDataAvailable(void)
//
// This function checks if HET UART RX data is available and ready for
// read out.
//------------------------------------------------------------------------------
char HetUARTRxDataAvailable(void)
{
  unsigned int Tmp = HET_RX_Buffer_0.memory.data_word >> 16;

  if (Tmp & 0x100)                              // Stop bit detected?
    return 1;                                   // Yes, data available
  else
    return 0;                                   // No data received
}
//------------------------------------------------------------------------------
// char HetUARTTxBufferEmpty(void)
//
// This function returns true if the HET UART TX buffer can be written to.
//------------------------------------------------------------------------------
char HetUARTTxBufferEmpty(void)
{
  if ((HET_TX_BitCtr_0.memory.data_word >> 5) & 0xFFFFF)
    return 0;                                   // TX in progress
  else
    return 1;                                   // TX buffer empty
}
//------------------------------------------------------------------------------
void main(void)
{
  PCR = 0x00;                                   // Disable peripherals
  GCR = ZPLL_MULT4;                             // Enable PLL multiply-by-4
  PCR = CLKDIV_2;                               // ICLK=SYSCLK/2
                                                // (to not violate max. ICLK)
  PCR |= PENABLE;                               // Enable peripherals
                                                // AFTER setting CLKDIV

  HETDOUT = 0x01000000;                         // Output HIGH on HET24
                                                // (RS-232 idle state)
  HETDIR  = 0x7FFFFFFF;                         // Set all HETx
                                                // but HET31 as outputs

  // Copy HET instructions to HET RAM
  MemCopy32((void *) &e_HETPROGRAM0_UN, (void *) HET_INIT0_PST,
            sizeof(HET_INIT0_PST));

  HETGCR = CLK_MASTER + IGNORE_SUSPEND;         // HET Master Mode,
                                                // Ignore SW BP

  // Set HET PFR register
  // fSYSCLK = 48MHz
  // fHR = fSYSCLK / 4 = 12Mhz
  // fLR = fHR / 32 = 375kHz
  HETPFR = LRPRES_FACTOR_32 + HRPRES_FACTOR_4;

  HETGCR |= ON;                                 // Enable HET

  while (1)                                     // Loop forever...
  {
    if (HetUARTRxDataAvailable())               // Data available?
      HetUARTPutByte(HetUARTGetByte());         // Read and send back...
  }
}
