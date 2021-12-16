// Copyright (c) 2018-2019 The VERGE Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * Functionality for communicating with Tor.
 */
#ifndef VERGE_TORCONTROLLER_HPP
#define VERGE_TORCONTROLLER_HPP

#include <atomic>
#include <deque>
#include <iostream>
#include <memory>
#include <stdint.h>
#include <string>
#include <thread>

#include <boost/filesystem.hpp>
#include <boost/thread.hpp>

#include <QString>

/** Default Port to run tor entry node on **/
static const unsigned int DEFAULT_TOR_PORT = 9090;

/** Default Port for handling tor's control port **/
static const unsigned int DEFAULT_TOR_CONTROL_PORT = 9051;

char* convert_str(const std::string& s);

void run_tor(QString dataDirPath);

/**
 * Initializes a new tor thread within a new thread
 **/
void InitalizeTorThread(QString dataDirPath);

/**
 * Stops the known thread and tries to kill it
 **/
void StopTorController();

/**
 * Internally starts within the new thread
 **/
void StartTorController(QString dataDirPath);

#endif /* VERGE_TORCONTROLLER_HPP */
