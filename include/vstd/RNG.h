/*
 * RNG.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

namespace vstd
{

typedef std::function<int64_t()> TRandI64;
typedef std::function<double()> TRand;

class RNG
{
public:

	virtual ~RNG() = default;

	virtual TRandI64 getInt64Range(int64_t lower, int64_t upper) = 0;

	virtual TRand getDoubleRange(double lower, double upper) = 0;
};

} // namespace vstd
