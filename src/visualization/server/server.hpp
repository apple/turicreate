/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <visualization/server/plot.hpp>

#include <memory>
#include <unordered_map>

namespace turi {
namespace visualization {

    class WebServer {
    public:
        typedef std::unordered_map< std::string, Plot > plot_map;

        static WebServer& get_instance();
        ~WebServer();

        // Generates and returns the URL to a given Plot object.
        // Spins up the web server lazily, if needed.
        static std::string get_url_for_plot(const Plot& plot);

    private:
        WebServer();

        class Impl;
        plot_map m_plots;
        std::unique_ptr<Impl> m_impl;

        std::string add_plot(const Plot& plot);

    };

    extern std::string VISUALIZATION_WEB_SERVER_ROOT_DIRECTORY;

}}
