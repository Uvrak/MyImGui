#include "SvgFileWriter.h"

#include <fstream>

bool SvgFileWriter::save(
    const std::string& filename,
    const PixelImage& image
)
{
    if (image.width() <= 0 ||
        image.height() <= 0)
    {
        return false;
    }

    std::ofstream file(
        filename,
        std::ios::trunc
    );

    if (!file)
    {
        return false;
    }

    file
        << "<svg xmlns=\"http://www.w3.org/2000/svg\" "
        << "width=\"" << image.width() << "\" "
        << "height=\"" << image.height() << "\" "
        << "viewBox=\"0 0 "
        << image.width() << ' '
        << image.height() << "\" "
        << "shape-rendering=\"crispEdges\">\n";

    for (int y = 0;
        y < image.height();
        ++y)
    {
        for (int x = 0;
            x < image.width();
            ++x)
        {
            const PixelColor& color =
                image.pixel(x, y);

            if (color.alpha == 0)
            {
                continue;
            }

            file
                << "  <rect x=\"" << x
                << "\" y=\"" << y
                << "\" width=\"1\" height=\"1\""
                << " fill=\"rgb("
                << static_cast<int>(color.red) << ','
                << static_cast<int>(color.green) << ','
                << static_cast<int>(color.blue) << ")\"";

            if (color.alpha < 255)
            {
                file
                    << " fill-opacity=\""
                    << static_cast<float>(color.alpha) /
                    255.0f
                    << "\"";
            }

            file << "/>\n";
        }
    }

    file << "</svg>\n";

    return file.good();
}