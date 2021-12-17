#pragma once

#include "mvItemRegistry.h"
#include "mvContext.h"
#include "mvModule_DearPyGui.h"
#include <string>

namespace Marvel {

    class mvColorMapScale : public mvAppItem
    {

    public:

        static void InsertParser(std::map<std::string, mvPythonParser>* parsers);

        MV_APPLY_WIDGET_REGISTRATION(mvAppItemType::mvColorMapScale, add_colormap_scale)

    public:

        explicit mvColorMapScale(mvUUID uuid);

        void draw(ImDrawList* drawlist, float x, float y) override;
        void handleSpecificKeywordArgs(PyObject* dict) override;
        void getSpecificConfiguration(PyObject* dict) override;
        void applySpecificTemplate(mvAppItem* item) override;
        void setColorMap(ImPlotColormap colormap);

    private:

        double          _scale_min = 0;
        double          _scale_max = 0;
        ImPlotColormap  _colormap = 0;

    };

}