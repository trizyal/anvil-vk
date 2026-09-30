// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "stox.h"

#include <string>
#include <stdexcept>

#include "Ensure.h"
#include "Logger.h"

namespace tml
{
    std::optional<int> SToInt(const std::string& str)
    {
        try
        {
            return std::stoi(str);
        }
        catch (const std::invalid_argument&)
        {
            LOG_WARN("SToInt failed: '{}' is not a valid number.", str);
        }
        catch (const std::out_of_range&)
        {
            LOG_WARN("SToInt failed: '{}' is out of range for a 32-bit integer.", str);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("SToInt failed unexpectedly on '{}' : {}", str, e.what());
            ENSURE(false, "Unexpected scope reached.");
        }

        return std::nullopt;
    }

    std::optional<float> SToFloat(const std::string& str)
    {
        try
        {
            return std::stoi(str);
        }
        catch (const std::invalid_argument&)
        {
            LOG_WARN("SToFloat failed: '{}' is not a valid number.", str);
        }
        catch (const std::out_of_range&)
        {
            LOG_WARN("SToFloat failed: '{}' is out of range for a 32-bit float.", str);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("SToFloat failed unexpectedly on '{}' : {}", str, e.what());
            ENSURE(false, "Unexpected scope reached.");
        }

        return std::nullopt;
    }
}
