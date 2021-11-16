
// Copyright 2021, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JTABLEPRINTER_H
#define JANA2_JTABLEPRINTER_H

#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>

class JTablePrinter {

    int current_column = 0;
    int current_row = 0;

public:

    enum class Justify {Left, Center, Right};
    enum class RuleStyle {Across, Broken, None};

    struct Column {
        std::string header;
        std::vector<std::string> values;
        Justify justify = Justify::Left;
        int desired_width;
        int contents_width;
        bool use_desired_width = false;
    };

    std::string title;
    std::vector<Column> columns;
    RuleStyle top_rule = RuleStyle::Across;
    RuleStyle header_rule = RuleStyle::Broken;
    RuleStyle bottom_rule = RuleStyle::Across;

    int indent = 2;
    int cell_margin = 2;

    JTablePrinter::Column& AddColumn(std::string header, Justify justify=Justify::Left, int desired_width=0);
    void FormatCell(std::ostream& os, std::string contents, int max_width, Justify justify);
    void Render(std::ostream& os);

    template <typename T> JTablePrinter& operator|(T);


};

template <typename T>
JTablePrinter& JTablePrinter::operator|(T cell) {
    std::ostringstream ss;
    ss << cell;
    return (*this) | ss.str();
}

template <>
inline JTablePrinter& JTablePrinter::operator|(std::string cell) {
    auto len = cell.size();
    if (columns[current_column].contents_width < len) {
	columns[current_column].contents_width = len;
    }
    columns[current_column].values.push_back(cell);
    current_column += 1;
    if (current_column >= columns.size()) {
	current_column = 0;
	current_row += 1;
    }
    return *this;
}

#endif //JANA2_JTABLEPRINTER_H
