/**
 * @file uart_cmd.h
 * @brief UART command processing module interface
 *
 * This module provides functionality for handling UART communications in a 
 * separate thread, allowing the main application to continue execution while 
 * UART commands are processed asynchronously.
 *
 * @author Gabriel Kis
 * @date 2025-04-01
 */

#ifndef UART_CMD_H_
#define UART_CMD_H_

/**
 * @brief Initialize and start the UART command processing thread
 *
 * This function creates and launches a new thread that handles UART communication.
 * The thread will:
 * - Set up the necessary UART hardware
 * - Configure interrupt handlers for UART events
 * - Process incoming UART data in a continuous loop
 * - Handle commands received via UART
 *
 * @note The thread runs concurrently with the main application. Any shared resources
 * accessed by both the main application and the UART thread should be protected with
 * appropriate synchronization mechanisms.
 *
 * @pre The system kernel must be initialized
 * @pre The UART hardware must be available
 *
 * @return None
 *
 * @see uart_thread_stop
 *
 * @code
 * #include "uart_cmd.h"
 *
 * int main(void)
 * {
 *     // Initialize other system components
 *     
 *     // Start the UART command processing thread
 *     uart_thread_start();
 *     
 *     // Continue with main application logic
 *     while (1) {
 *         // Main application loop
 *     }
 *     
 *     return 0;
 * }
 * @endcode
 */
void uart_thread_start(void);

#endif /* UART_CMD_H_ */
