/**
 * Copyright 2013-2021 Software Radio Systems Limited
 *
 * This file is part of srsLTE.
 *
 * srsLTE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsLTE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

/*!
 * \file utils_avx2.h
 * \brief Declarations of AVX2-related quantities and functions.
 * \author David Gregoratti
 * \date 2020
 *
 * \copyright Software Radio Systems Limited
 *
 */

#ifndef SRSLTE_UTILS_AVX2_H
#define SRSLTE_UTILS_AVX2_H

#define SRSLTE_AVX2_B_SIZE 32    /*!< \brief Number of packed bytes in an AVX2 instruction. */
#define SRSLTE_AVX2_B_SIZE_LOG 5 /*!< \brief \f$\log_2\f$ of \ref SRSLTE_AVX2_B_SIZE. */

#endif // SRSLTE_UTILS_AVX2_H
