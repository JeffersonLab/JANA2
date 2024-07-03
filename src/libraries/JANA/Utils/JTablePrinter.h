
// Copyright 2021, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#pragma once
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
    bool vertical_padding = 0; // Automatically turned on if cell contents overflow desired_width

    JTablePrinter::Column& AddColumn(std::string header, Justify justify=Justify::Left, int desired_width=0);
    void FormatLine(std::ostream& os, std::string contents, int max_width, Justify justify) const;
    void Render(std::ostream& os) const;
    std::string Render() const;
    static std::vector<std::string> SplitContents(std::string contents, size_t max_width);
    static std::vector<std::string> SplitContentsByNewlines(std::string contents);
    static std::vector<std::string> SplitContentsBySpaces(std::string contents, size_t max_width);

    template <typename T> JTablePrinter& operator|(T);


};

std::ostream& operator<<(std::ostream& os, const JTablePrinter& t);

template <typename T>
JTablePrinter& JTablePrinter::operator|(T cell) {
    std::ostringstream ss;
    ss << cell;
    return (*this) | ss.str();
}

template <>
inline JTablePrinter& JTablePrinter::operator|(std::string cell) {
    auto len = cell.size();
    auto& col = columns[current_column];
    if ((size_t) col.contents_width < len) {
        col.contents_width = len;
    }
    if (len > (size_t) col.desired_width && col.desired_width != 0) {
        vertical_padding = true;
        // columns[current_column].use_desired_width = true; // TODO: use_desired_width is broken
    }
    col.values.push_back(cell);
    current_column += 1;
    if ((size_t) current_column >= columns.size()) {
	current_column = 0;
	current_row += 1;
    }
    return *this;
}

