//
// Copyright (C) YuqiaoZhang(HanetakaChou)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#include <QtCore/QTextCodec>
#include <QtCore/QDir>
#include <QtCore/QtPlugin>
#include <QtGui/QFontDatabase>
#include <QtGui/QIcon>
#include <QtGui/QImageReader>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QFileIconProvider>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cinttypes>
#include <vector>
#include <string>
#include <algorithm>
#include <unistd.h>
#include <locale.h>
#include <errno.h>
#include "../../Brioche-ImGui/misc/fonts/noto_sans_ttf.h"
#include "../../Brioche-ImGui/misc/fonts/noto_sans_cjk_jp_ttf.h"
#include "../../Brioche-ImGui/misc/fonts/noto_sans_symbols_ttf.h"
#include "../../Brioche-ImGui/misc/fonts/noto_sans_symbols2_ttf.h"
#include "../build-linux/resource.h"

Q_IMPORT_PLUGIN(QXcbIntegrationPlugin)
Q_IMPORT_PLUGIN(QComposePlatformInputContextPlugin)
Q_IMPORT_PLUGIN(QSvgIconPlugin)
Q_IMPORT_PLUGIN(QGifPlugin)
Q_IMPORT_PLUGIN(QJpegPlugin)
Q_IMPORT_PLUGIN(QICOPlugin)
Q_IMPORT_PLUGIN(QSvgPlugin)
Q_IMPORT_PLUGIN(QICNSPlugin)
Q_IMPORT_PLUGIN(QTgaPlugin)
Q_IMPORT_PLUGIN(QTiffPlugin)
Q_IMPORT_PLUGIN(QWbmpPlugin)
Q_IMPORT_PLUGIN(QWebpPlugin)

class ImageReaderIconProvider : public QFileIconProvider
{
public:
    QIcon icon(const QFileInfo &info) const override
    {
        if (info.isFile())
        {
            QImageReader image_reader(info.absoluteFilePath());
            image_reader.setAutoTransform(true);
            image_reader.setScaledSize(QSize(128, 128));
            if (image_reader.canRead())
            {
                QImage image = image_reader.read();
                if (!image.isNull())
                {
                    return QIcon(QPixmap::fromImage(image));
                }
            }
        }

        return QFileIconProvider::icon(info);
    }
};

int main(int argc, char **argv)
{
    // locale
    {
        char *res_set_locale_c_utf8 = setlocale(LC_ALL, "C");
        assert(NULL != res_set_locale_c_utf8);
    }

    // QT_QPA_FONTDIR
    {
        char dir_name[4096] = {};
        {
            ssize_t res_read_link = readlink("/proc/self/exe", dir_name, (sizeof(dir_name) / sizeof(dir_name[0])));
            assert(-1 != res_read_link);

            assert(res_read_link < (sizeof(dir_name) / sizeof(dir_name[0])));

            for (ssize_t index_plus_one = res_read_link; index_plus_one > 0; --index_plus_one)
            {
                ssize_t index = index_plus_one - 1;

                if ('/' != dir_name[index])
                {
                    dir_name[index] = '\0';
                }
                else
                {
                    break;
                }
            }
        }

        int res_set_env_qt_qpa_fontdir = setenv("QT_QPA_FONTDIR", dir_name, 1);
        assert(0 == res_set_env_qt_qpa_fontdir);
    }

    {
        QTextCodec *text_codec_utf8 = QTextCodec::codecForName("UTF-8");
        assert(NULL != text_codec_utf8);

        QTextCodec::setCodecForLocale(text_codec_utf8);
    }

    bool result_open_file;
    std::vector<char> result_name_filer_index_text;
    std::vector<char> result_file_name;
    {
        int framework_init_argc = 1;
        char *framework_init_argv[] = {argv[0], NULL};
        QApplication framework_init(framework_init_argc, framework_init_argv);

        {
            QStringList families;
            families.append(QFontDatabase::applicationFontFamilies(QFontDatabase::addApplicationFontFromData(QByteArray::fromRawData(reinterpret_cast<char *>(brx_imgui_font_asset_get_noto_sans_ttf_data_base()), brx_imgui_font_asset_get_noto_sans_ttf_data_size()))));
            families.append(QFontDatabase::applicationFontFamilies(QFontDatabase::addApplicationFontFromData(QByteArray::fromRawData(reinterpret_cast<char *>(brx_imgui_font_asset_get_noto_sans_cjk_jp_ttf_data_base()), brx_imgui_font_asset_get_noto_sans_cjk_jp_ttf_data_size()))));
            families.append(QFontDatabase::applicationFontFamilies(QFontDatabase::addApplicationFontFromData(QByteArray::fromRawData(reinterpret_cast<char *>(brx_imgui_font_asset_get_noto_sans_symbols_ttf_data_base()), brx_imgui_font_asset_get_noto_sans_symbols_ttf_data_size()))));
            families.append(QFontDatabase::applicationFontFamilies(QFontDatabase::addApplicationFontFromData(QByteArray::fromRawData(reinterpret_cast<char *>(brx_imgui_font_asset_get_noto_sans_symbols2_ttf_data_base()), brx_imgui_font_asset_get_noto_sans_symbols2_ttf_data_size()))));

            QFont font = QApplication::font();
            font.setFamilies(families);

            QApplication::setFont(font);
        }

        QFileDialog file_open_dialog(NULL, QString::fromUtf8("Brioche File Open Dialog"));

        file_open_dialog.setOption(QFileDialog::DontUseNativeDialog, true);

        file_open_dialog.setIconProvider(new ImageReaderIconProvider);

        file_open_dialog.setWindowIcon(QIcon(QPixmap::fromImage(QImage(reinterpret_cast<uchar const *>(ICON_BRX_PUPPET_DATA), ICON_BRX_PUPPET_WIDTH, ICON_BRX_PUPPET_HEIGHT, sizeof(uint32_t) * ICON_BRX_PUPPET_WIDTH, QImage::Format_ARGB32))));

        file_open_dialog.setFileMode(QFileDialog::ExistingFile);

        if (argc >= 2)
        {
            QString file_name = QString::fromUtf8(argv[1]);
            file_open_dialog.selectFile(file_name);
        }

        QStringList name_filters;
        if (argc >= 3)
        {
            long long in_name_filer_index = std::strtoll(argv[2], NULL, 10);

            for (int i = 3; i < argc; ++i)
            {
                // e.g. "Video (*.mp4 *.mkv)"
                name_filters.push_back(QString::fromUtf8(argv[i]));
            }

            file_open_dialog.setNameFilters(name_filters);

            if ((in_name_filer_index >= 0) && (in_name_filer_index < name_filters.size()))
            {
                file_open_dialog.selectNameFilter(name_filters[in_name_filer_index]);
            }
        }

        int res_exec = file_open_dialog.exec();

        assert(result_name_filer_index_text.empty());
        {
            int out_name_filer_index = name_filters.indexOf(file_open_dialog.selectedNameFilter());

            char out_name_filer_index_text[] = {"-2147483647"};
            std::snprintf(out_name_filer_index_text, sizeof(out_name_filer_index_text) / sizeof(out_name_filer_index_text[0]), "%" PRId32, static_cast<int32_t>(out_name_filer_index));

            size_t out_name_filer_index_text_length = std::strlen(out_name_filer_index_text);
            result_name_filer_index_text.resize(out_name_filer_index_text_length + 1);
            std::memcpy(result_name_filer_index_text.data(), out_name_filer_index_text, sizeof(char) * (out_name_filer_index_text_length + 1));
        }

        assert(result_file_name.empty());
        if (QDialog::Accepted == res_exec)
        {
            result_open_file = true;

            QStringList out_file_names = file_open_dialog.selectedFiles();
            if (!out_file_names.isEmpty())
            {
                QByteArray out_file_name = out_file_names.first().toUtf8();
                result_file_name.resize(out_file_name.size() + 1);
                std::memcpy(result_file_name.data(), out_file_name.constData(), out_file_name.size());
                result_file_name[out_file_name.size()] = '\0';
            }
        }
        else
        {
            result_open_file = false;
            result_file_name.resize(1);
            result_file_name[0] = '\0';
        }
    }

    bool write_name_filter_index_text;
    if (result_open_file)
    {
        assert(!result_name_filer_index_text.empty());

        write_name_filter_index_text = true;

        size_t offset = 0U;
        while (offset < result_name_filer_index_text.size())
        {
            ssize_t size_write = write(STDOUT_FILENO, result_name_filer_index_text.data() + offset, result_name_filer_index_text.size() - offset);
            if (size_write > 0)
            {
                offset += static_cast<uint32_t>(size_write);
            }
            else if ((-1 == size_write) && (EINTR == errno))
            {
                // Do Nothing
            }
            else
            {
                write_name_filter_index_text = false;
                break;
            }
        }

        assert(result_name_filer_index_text.size() == offset);
    }
    else
    {
        write_name_filter_index_text = false;
    }

    bool write_file_name;
    if (result_open_file)
    {
        assert(!result_file_name.empty());

        write_file_name = true;

        size_t offset = 0U;
        while (offset < result_file_name.size())
        {
            ssize_t size_write = write(STDOUT_FILENO, result_file_name.data() + offset, result_file_name.size() - offset);
            if (size_write > 0)
            {
                offset += static_cast<uint32_t>(size_write);
            }
            else if ((-1 == size_write) && (EINTR == errno))
            {
                // Do Nothing
            }
            else
            {
                write_file_name = false;
                break;
            }
        }

        assert(result_file_name.size() == offset);
    }
    else
    {
        write_file_name = false;
    }

    return ((result_open_file && (write_name_filter_index_text && write_file_name)) ? 0 : -1);
}
