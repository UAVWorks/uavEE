////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2018 University of Illinois Board of Trustees
//
// This file is part of uavEE.
//
// uavAP is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// uavAP is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
////////////////////////////////////////////////////////////////////////////////
/*
 * Codec.h
 *
 *  Created on: Jul 28, 2017
 *      Author: mircot
 */

#ifndef SRC_SIMULATION_CODEC_H_
#define SRC_SIMULATION_CODEC_H_
#include <cstddef>
#include <string>

std::string
decode(const std::string& encoded);

std::string
encode(const std::string& source);

#endif /* SRC_SIMULATION_CODEC_H_ */
