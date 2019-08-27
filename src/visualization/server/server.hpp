/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <visualization/server/plot.hpp>

#include <memory>
#include <unordered_map>

namespace turi {

class sframe_reader;
class unity_sframe;

namespace visualization {

    class WebServer {
    public:
        struct table {
            table(const std::shared_ptr<unity_sframe>& sf, std::unique_ptr<sframe_reader> reader, const std::string& title);
            std::shared_ptr<unity_sframe> sf;
            std::unique_ptr<sframe_reader> reader;
            std::string title;
        };

        typedef std::unordered_map< std::string, Plot > plot_map;
        typedef std::vector< table > table_vector;

        static WebServer& get_instance();
        ~WebServer();

        // Returns the ID of the added element
        std::string add_plot(const Plot& plot);
        std::string add_table(const std::shared_ptr<unity_sframe>& table, const std::string& title);

        // Generates and returns the URL (optionally to a given visualization object).
        // Spins up the web server lazily, if needed.
        static std::string get_base_url();
        static std::string get_url_for_plot(const Plot& plot);
        static std::string get_url_for_table(const std::shared_ptr<unity_sframe>& table, const std::string& title);

    private:
        WebServer();

        class Impl;
        plot_map m_plots;
        table_vector m_tables;
        std::unique_ptr<Impl> m_impl;
    };

    extern std::string VISUALIZATION_WEB_SERVER_ROOT_DIRECTORY;

}}
