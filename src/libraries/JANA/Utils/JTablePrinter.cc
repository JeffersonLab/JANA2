
// Copyright 2021, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "JTablePrinter.h"
#include "JANA/JLogger.h"

std::vector<std::string> JTablePrinter::SplitContents(std::string contents, size_t max_width) {
    auto split_by_newlines = SplitContentsByNewlines(contents);
    if (max_width == 0) return split_by_newlines;

    std::vector<std::string> results;
    for (auto& s: split_by_newlines) {
        auto split_by_spaces = SplitContentsBySpaces(s, max_width);
        for (auto& ss : split_by_spaces) {
            results.push_back(ss);
        }
    }
    return results;
}

std::vector<std::string> JTablePrinter::SplitContentsByNewlines(std::string contents) {
    std::vector<std::string> split_by_newlines;
    size_t contents_width = contents.size();
    size_t line_start = 0;
    size_t line_end = line_start;
    while (line_end < contents_width) {
        if (contents[line_end] == '\n') {
            split_by_newlines.push_back(contents.substr(line_start, line_end - line_start));
            line_start = line_end + 1;
            line_end = line_start;
        }
        line_end += 1;
    }
    split_by_newlines.push_back(contents.substr(line_start, contents_width - line_start));
    return split_by_newlines;
}

std::vector<std::string> JTablePrinter::SplitContentsBySpaces(std::string contents, size_t max_width) {

    std::vector<std::string> split_by_spaces;
    if (contents.size() <= max_width) {
        split_by_spaces.push_back(contents);
        return split_by_spaces;
    }

    size_t split_start = 0;
    size_t split_end = 0;
    size_t contents_end = contents.size();
    for (size_t i = 0; i < contents_end; ++i) {
        if (contents[i] == ' ') {
            // std::cout << "Found ' ' on i=" << i << std::endl;
            split_end = i;
            // std::cout << "Setting split = [" << split_start << ", " << split_end << "]" << std::endl;
        }
        if (i - split_start == max_width) {
            // std::cout << "Hit max_width on i=" << i << std::endl;
            if (split_end == split_start) {
                split_by_spaces.push_back(contents.substr(split_start, i - split_start));
                // std::cout << "Pushing back " << split_by_spaces.back() << " (no split!)" << std::endl;
                split_start = i;
                split_end = i; // Because we didn't find a split, we don't want to lose any characters here
                // std::cout << "Setting split = [" << split_start << ", " << split_end << "]" << std::endl;
            }
            else {
                split_by_spaces.push_back(contents.substr(split_start, split_end - split_start));
                // std::cout << "Pushing back " << split_by_spaces.back() << " (found split!)" << std::endl;
                split_start = split_end + 1; // Because we found a split, we want to lose the extra space
                split_end = split_start;
                // std::cout << "Setting split = [" << split_start << ", " << split_end << "]" << std::endl;
            }
        }
    }
    split_end = contents_end;
    // std::cout << "Setting split = [" << split_start << ", " << split_end << "]" << std::endl;

    split_by_spaces.push_back(contents.substr(split_start, split_end - split_start));
    // std::cout << "Pushing back " << split_by_spaces.back() << "(cleanup)" << std::endl;
    return split_by_spaces;
}

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


void JTablePrinter::FormatLine(std::ostream& os, std::string contents, int max_width, Justify justify) {
    auto cs = contents.size();

    if (justify == Justify::Left) {
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

    size_t total_rows = current_row;
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
        FormatLine(os, col.header, col.use_desired_width ? col.desired_width : col.contents_width, Justify::Center);
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
    for (size_t row = 0; row < total_rows; ++row) {
        std::vector<std::vector<std::string>> lines;
        size_t line_count = 1;
        for (const auto& col : columns) {
            auto split = SplitContents(col.values[row], col.desired_width);
            lines.push_back(split);
            if (split.size() > line_count) line_count = split.size();
        }
        if (vertical_padding) line_count += 1;

        for (size_t line=0; line < line_count; ++line) {
            for (int i = 0; i<indent; ++i) os << " ";

            size_t col_count = columns.size();
            for (size_t col = 0; col < col_count; ++col) {
                auto& column = columns[col];
                if (line < lines[col].size()) {
                    FormatLine(os, lines[col][line], column.use_desired_width ? column.desired_width : column.contents_width, column.justify);
                }
                else {
                    FormatLine(os, "", column.use_desired_width ? column.desired_width : column.contents_width, column.justify);
                }
                for (int i=0; i<cell_margin; ++i) os << " ";
            }
            os << std::endl;

        }
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

