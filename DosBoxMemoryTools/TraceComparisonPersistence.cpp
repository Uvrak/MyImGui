#include "TraceComparisonPersistence.h"

#include <fstream>
#include <utility>

namespace DosBoxMemoryTools::TraceComparisonPersistence
{
    bool savePath(
        const std::filesystem::path& path,
        const std::string& sourceFilename
    ) noexcept
    {
        try
        {
            if (!path.parent_path().empty())
            {
                std::filesystem::create_directories(
                    path.parent_path()
                );
            }

            std::ofstream file(
                path,
                std::ios::trunc
            );

            if (!file)
            {
                return false;
            }

            file << sourceFilename;

            return file.good();
        }
        catch (const std::exception&)
        {
            return false;
        }
    }

    bool loadPath(
        const std::filesystem::path& path,
        std::string& sourceFilename
    ) noexcept
    {
        try
        {
            std::ifstream file(
                path
            );

            if (!file)
            {
                return false;
            }

            std::string filename;

            std::getline(
                file,
                filename
            );

            if (filename.empty())
            {
                return false;
            }

            sourceFilename =
                std::move(filename);

            return true;
        }
        catch (const std::exception&)
        {
            return false;
        }
    }
}