
// Copyright 2021, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "JTablePrinter.h"


JTablePrinter::Column& JTablePrinter::AddColumn(std::string header, Justify justify, int desired_width) {
    columns.emplace_back();
    Column& col = columns.back();
    col.header = std::move(header);
    col.contents_width = col.header.size();
    col.justify = justify;
    col.desired_width = desired_width;
    col.use_desired_width = (desired_width != 0);
    return col;
}


void JTablePrinter::FormatCell(std::ostream& os, std::string contents, int max_width, Justify justify) {
    auto cs = contents.size();
    if (cs > max_width) {
        os << contents.substr(0, max_width-2) << "\u2026";
    }
    else if (justify == Justify::Left) {
        os << std::left << std::setw(max_width) << contents;
    }
    else if (justify == Justify::Right) {
        os << std::right << std::setw(max_width) << contents;
    }
    else {
        int lpad = (max_width-cs)/2; // center slightly to the left
        int rpad = (max_width-cs) - lpad;
        for (int i=0; i<lpad; ++i) os << " ";
        os << contents;
        for (int i=0; i<rpad; ++i) os << " ";
    }
}

void JTablePrinter::Render(std::ostream& os) {

    int total_rows = current_row;
    current_row = 0;

    // Calculate table width
    int table_width = 0;
    for (const Column& col : columns) {
        table_width += col.use_desired_width ? col.desired_width : col.contents_width;
    }
    table_width += (columns.size()-1)*cell_margin;

    // Print top rule
    if (top_rule == RuleStyle::Broken) {
        for (int i = 0; i<indent; ++i) os << " ";
        for (const auto& col : columns) {
            int underline_len = (col.use_desired_width)?col.desired_width:col.contents_width;
            for (int i=0; i<underline_len; ++i) os << "-";
            for (int i=0; i<cell_margin; ++i) os << " ";
        }
        os << std::endl;
    }
    else if (top_rule == RuleStyle::Across) {
        for (int i = 0; i<indent; ++i) os << " ";
        for (int i=0; i<table_width; ++i) os << "-";
        os << std::endl;
    }

    // Print headings
    for (int i = 0; i<indent; ++i) os << " ";
    for (const auto& col : columns) {
        // os << col.header;
        FormatCell(os, col.header, col.use_desired_width?col.desired_width:col.contents_width, Justify::Center);
        for (int i=0; i<cell_margin; ++i) os << " ";
    }
    os << std::endl;

    // Print header rule
    if (header_rule == RuleStyle::Across) {
        for (int i = 0; i<indent; ++i) os << " ";
        for (int i = 0; i<table_width; ++i) os << "-";
        os << std::endl;
    }
    else if (header_rule == RuleStyle::Broken) {
        for (int i = 0; i<indent; ++i) os << " ";
        for (const auto& col : columns) {
            int underline_len = (col.use_desired_width)?col.desired_width:col.contents_width;
            for (int i=0; i<underline_len; ++i) os << "-";
            for (int i=0; i<cell_margin; ++i) os << " ";
        }
        os << std::endl;
    }

    // Print rows
    for (int row = 0; row < total_rows; ++row) {
        for (int i = 0; i<indent; ++i) os << " ";
        for (const auto& col : columns) {
            FormatCell(os, col.values[row], col.use_desired_width?col.desired_width:col.contents_width, col.justify);
            for (int i=0; i<cell_margin; ++i) os << " ";
        }
        os << std::endl;
    }

    // Print bottom rule
    if (bottom_rule == RuleStyle::Broken) {
        for (int i = 0; i<indent; ++i) os << " ";
        for (const auto& col : columns) {
            int underline_len = (col.use_desired_width)?col.desired_width:col.contents_width;
            for (int i=0; i<underline_len; ++i) os << "-";
            for (int i=0; i<cell_margin; ++i) os << " ";
        }
        os << std::endl;
    }
    else if (bottom_rule == RuleStyle::Across) {
        for (int i = 0; i<indent; ++i) os << " ";
        for (int i = 0; i<table_width; ++i) os << "-";
        os << std::endl;
    }
}

