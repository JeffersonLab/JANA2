#!/usr/bin/env python3

import sys
import re
from datetime import datetime, timedelta
from collections import defaultdict


class Drawing:
    def __init__(self, width, height, profile="tiny"):
        self.lines = [
            f"""<?xml version="1.0" encoding="utf-8" ?>\n""",
            f"""<svg baseProfile="{profile}" width="{width}" height="{height}" version="1.2" xmlns="http://www.w3.org/2000/svg" xmlns:ev="http://www.w3.org/2001/xml-events" xmlns:xlink="http://www.w3.org/1999/xlink"><defs />"""
        ]

    def write(self, output_filename="timeline.svg"):
        self.lines.append("</svg>")
        output_file = open(output_filename, 'w')
        output_file.writelines(self.lines)
        output_file.close()

    def add_rect(self, x, y, width, height, fill="lightgray", stroke="darkgray", stroke_width=1, mouseover_text=""):
        if not mouseover_text:
            self.lines.append(f"""<rect fill="{fill}" height="{height}" stroke="{stroke}" stroke-width="{stroke_width}" width="{width}" x="{x}" y="{y}" />\n""")
        else:
            self.lines.append(f"""<rect fill="{fill}" height="{height}" stroke="{stroke}" stroke-width="{stroke_width}" width="{width}" x="{x}" y="{y}">\n""")
            self.lines.append(f"""    <title>{mouseover_text}</title>\n""")
            self.lines.append(f"""</rect>\n\n""")

    def add_text(self, text, x, y, fill="black", font_size=8):
        self.lines.append(f"""<text fill="{fill}" font-size="{font_size}" x="{x}" y="{y}">{text}</text>\n\n""")


def parse_logfile(input_filename="log.txt"):

    # Parse the logs and store intervals
    thread_history = defaultdict(list)  # Key: thread_id, Value: list of (start_time, end_time, processor_name, event_nr, result)
    region_history = defaultdict(list)  # Key: thread_id, Value: list of (start_time, end_time, region_name, event_nr)
    barrier_history = [] # [(released_timestamp, finished_timestamp)]

    start_times = {}  # Key: (thread_id, processor_name), Value: start_time

    # Define a regular expression to parse the log lines
    source_start_pattern = re.compile(r"^(\d{2}):(\d{2}):(\d{2})\.(\d{3}) #(\d+) \[debug\] Executing arrow (\w+)$")
    source_finish_noemit_pattern = re.compile(r"^(\d{2}):(\d{2}):(\d{2})\.(\d{3}) #(\d+) \[debug\] Executed arrow (\w+) with result (\w+)$")
    source_finish_emit_pattern = re.compile(r"^(\d{2}):(\d{2}):(\d{2})\.(\d{3}) #(\d+) \[debug\] Executed arrow (\w+) with result (\w+), emitting event# (\d+)$")
    source_finish_pending_pattern = re.compile(r"^(\d{2}):(\d{2}):(\d{2})\.(\d{3}) #(\d+) \[debug\] Executed arrow (\w+) with result (\w+), holding back barrier event# (\d+)$")
    processor_start_pattern = re.compile(r"^(\d{2}):(\d{2}):(\d{2})\.(\d{3}) #(\d+) \[debug\] Executing arrow (\w+) for event# (\d+)$")
    processor_finish_pattern = re.compile(r"^(\d{2}):(\d{2}):(\d{2})\.(\d{3}) #(\d+) \[debug\] Executed arrow (\w+) for event# (\d+)$")
    barrier_inflight_pattern = re.compile(r"^(\d{2}):(\d{2}):(\d{2})\.(\d{3}) #(\d+) \[debug\] JEventSourceArrow: Barrier event is in-flight$")
    barrier_finished_pattern = re.compile(r"^(\d{2}):(\d{2}):(\d{2})\.(\d{3}) #(\d+) \[debug\] JEventSourceArrow: Barrier event finished, returning to normal operation$")
    region_start_pattern = re.compile(r"^(\d{2}):(\d{2}):(\d{2})\.(\d{3}) #(\d+)  \[info\] Entering region ([\w:]+) for event# (\d+)$")
    region_finish_pattern = re.compile(r"^(\d{2}):(\d{2}):(\d{2})\.(\d{3}) #(\d+)  \[info\] Exited region ([\w:]+) for event# (\d+)$")

    with open(input_filename, "r") as log_file:
        for line in log_file:

            match = re.match(source_start_pattern, line.strip())
            if match:
                hours_str, mins_str, secs_str, millis_str, thread_id, processor_name = match.groups()
                millis = (((int(hours_str) * 60) + int(mins_str) * 60) + int(secs_str)) * 1000 + int(millis_str)
                start_times[(thread_id, processor_name)] = millis
                continue

            match = re.match(source_finish_noemit_pattern, line.strip())
            if match:
                hours_str, mins_str, secs_str, millis_str, thread_id, processor_name, result = match.groups()
                millis = (((int(hours_str) * 60) + int(mins_str) * 60) + int(secs_str)) * 1000 + int(millis_str)
                start_time = start_times.pop((thread_id, processor_name), None)
                if start_time:
                    thread_history[thread_id].append((start_time, millis, processor_name, None, result))
                continue
            
            match = re.match(source_finish_emit_pattern, line.strip())
            if match:
                hours_str, mins_str, secs_str, millis_str, thread_id, processor_name, result, event_nr = match.groups()
                millis = (((int(hours_str) * 60) + int(mins_str) * 60) + int(secs_str)) * 1000 + int(millis_str)
                start_time = start_times.pop((thread_id, processor_name), None)
                if start_time:
                    thread_history[thread_id].append((start_time, millis, processor_name, event_nr, result))
                continue
            
            match = re.match(source_finish_pending_pattern, line.strip())
            if match:
                hours_str, mins_str, secs_str, millis_str, thread_id, processor_name, result, event_nr = match.groups()
                millis = (((int(hours_str) * 60) + int(mins_str) * 60) + int(secs_str)) * 1000 + int(millis_str)
                start_time = start_times.pop((thread_id, processor_name), None)
                if start_time:
                    thread_history[thread_id].append((start_time, millis, processor_name, event_nr, result))
                continue
            
            match = re.match(processor_start_pattern, line.strip())
            if match:
                hours_str, mins_str, secs_str, millis_str, thread_id, processor_name, event_nr = match.groups()
                millis = (((int(hours_str) * 60) + int(mins_str) * 60) + int(secs_str)) * 1000 + int(millis_str)
                start_times[(thread_id, processor_name)] = millis
                continue
            
            match = re.match(processor_finish_pattern, line.strip())
            if match:
                hours_str, mins_str, secs_str, millis_str, thread_id, processor_name, event_nr = match.groups()
                millis = (((int(hours_str) * 60) + int(mins_str) * 60) + int(secs_str)) * 1000 + int(millis_str)
                start_time = start_times.pop((thread_id, processor_name), None)
                if start_time:
                    thread_history[thread_id].append((start_time, millis, processor_name, event_nr, result))
                continue
            
            match = re.match(region_start_pattern, line.strip())
            if match:
                hours_str, mins_str, secs_str, millis_str, thread_id, region_name, event_nr = match.groups()
                millis = (((int(hours_str) * 60) + int(mins_str) * 60) + int(secs_str)) * 1000 + int(millis_str)
                start_times[(thread_id, region_name)] = millis
                continue
            
            match = re.match(region_finish_pattern, line.strip())
            if match:
                hours_str, mins_str, secs_str, millis_str, thread_id, region_name, event_nr = match.groups()
                millis = (((int(hours_str) * 60) + int(mins_str) * 60) + int(secs_str)) * 1000 + int(millis_str)
                start_time = start_times.pop((thread_id, region_name), None)
                if start_time:
                    region_history[thread_id].append((start_time, millis, region_name, event_nr))
                else:
                    print("No matching enter region")
                continue
            
            match = re.match(barrier_inflight_pattern, line.strip())
            if match:
                hours_str, mins_str, secs_str, millis_str, thread_id = match.groups()
                millis = (((int(hours_str) * 60) + int(mins_str) * 60) + int(secs_str)) * 1000 + int(millis_str)
                start_times[()] = millis
                continue

            match = re.match(barrier_finished_pattern, line.strip())
            if match:
                hours_str, mins_str, secs_str, millis_str, thread_id = match.groups()
                millis = (((int(hours_str) * 60) + int(mins_str) * 60) + int(secs_str)) * 1000 + int(millis_str)
                start_time = start_times.pop((), None)
                if start_time:
                    barrier_history.append((start_time, millis))
                continue

    return (thread_history, region_history, barrier_history)



def create_svg(output_filename, all_thread_history, region_history, barrier_history):

    if not all_thread_history:
        print("Error: No thread history found")
        print()
        print("Make sure you use the following parameters:")
        print("  jana:loglevel=debug")
        print("  jana:log:show_threadstamp=1")
        return

    # Assign colors to processors
    processor_colors = {}
    #color_palette = ['#004E64', '#00A5CF', '#9FFFCB', '#25A18E', '#7AE582', '#FF69B4']
    color_palette = ['#004E64', '#00A5CF', '#25A18E', '#7AE582', '#FF69B4']
    color_index = 0

    # Figure out drawing coordinate system
    overall_start_time = min(start for history in all_thread_history.values() for (start,_,_,_,_) in history)
    overall_end_time = max(end for history in all_thread_history.values() for (_,end,_,_,_) in history)
    thread_count = len(all_thread_history)
    width=1000
    x_scale = width/(overall_end_time-overall_start_time)
    thread_height=40
    thread_y_padding=20
    height=thread_count * thread_height + (thread_count+1) * thread_y_padding


    # Create the SVG drawing
    dwg = Drawing(width=width, height=height)

    # Draw a rectangle for each processor run on each thread's timeline
    y_position = thread_y_padding

    for barrier_start,barrier_end in barrier_history:
        rect_start = (barrier_start-overall_start_time)*x_scale
        if (barrier_end == barrier_start): 
            rect_width=1
        else:
            rect_width = (barrier_end-barrier_start)*x_scale

        dwg.add_rect(x=rect_start, y=0,
                     width=rect_width, height=height,
                     fill="red", stroke="none", stroke_width=1)

    for thread_id, intervals in all_thread_history.items():

        thread_id = int(thread_id)
        y_position = (thread_y_padding + thread_height) * (thread_id-1) + thread_y_padding

        dwg.add_rect(x=0, y=y_position, width=1000, height=thread_height, stroke="lightgray", fill="lightgray")
        for start_time, end_time, processor_name, event_nr, result in intervals:
            # Calculate the position and width of the rectangle
            # Assign a unique color to each processor name
            if processor_name not in processor_colors:
                processor_colors[processor_name] = color_palette[color_index % len(color_palette)]
                color_index += 1

            # Draw the rectangle
            rect_start = (start_time-overall_start_time)*x_scale
            if (end_time == start_time): 
                rect_width=1
            else:
                rect_width = (end_time-start_time)*x_scale

            rect_stroke_color = "black"
            if (result == "ComeBackLater" and event_nr is None):
                rect_stroke_color = "gray"


            mouseover = "Arrow: " + processor_name + "\nEvent nr: " + str(event_nr) + "\nResult: " + result + "\nTime: "+ str(end_time-start_time) + "ms"

            dwg.add_rect(x=rect_start, y=y_position,
                         width=rect_width, height=thread_height,
                         fill=processor_colors[processor_name],
                         stroke=rect_stroke_color, stroke_width=1,
                         mouseover_text=mouseover)

            if (event_nr is not None):
                dwg.add_text(str(event_nr), x=rect_start+1, y=y_position+thread_height-1, fill="white", font_size=8)


    for thread_id, intervals in region_history.items():

        thread_id = int(thread_id)
        y_position = (thread_y_padding + thread_height) * (thread_id-1) + thread_y_padding

        for start_time, end_time, region_name, event_nr in intervals:
            # Calculate the position and width of the rectangle
            # Assign a unique color to each processor name
            if region_name not in processor_colors:
                processor_colors[region_name] = color_palette[color_index % len(color_palette)]
                color_index += 1

            # Draw the rectangle
            rect_start = (start_time-overall_start_time)*x_scale
            if (end_time == start_time): 
                rect_width=1
            else:
                rect_width = (end_time-start_time)*x_scale

            rect_stroke_color = "black"

            mouseover = "Region: " + region_name + "\nEvent nr: " + str(event_nr) + "\nTime: "+ str(end_time-start_time) + "ms"

            dwg.add_rect(x=rect_start, y=y_position+2,
                         width=rect_width, height=thread_height-12,
                         fill=processor_colors[region_name],
                         stroke=rect_stroke_color, stroke_width=1,
                         mouseover_text=mouseover)

            if (event_nr is not None):
                dwg.add_text(str(event_nr), x=rect_start+1, y=y_position+thread_height-12, fill="white", font_size=8)


    # Save the SVG file
    dwg.write(output_filename)



if __name__ == "__main__":

    if len(sys.argv) == 1:
        input_filename = "log.txt"
        output_filename = "timeline.svg"
    elif len(sys.argv) == 3:
        input_filename = sys.argv[1]
        output_filename = sys.argv[2]
    else:
        print("Error: Wrong number of args")
        print("Usage: python jana-plot-utilization.py INPUTFILE OUTPUTFILE")
        sys.exit(1)

    thread_history,region_history,barrier_history = parse_logfile(input_filename)
    #thread_history = {
    #    1103:[(0,1,"a"), (2,5,"b"), (6,8,"a")],
    #    219:[(0,3,"b"), (3,6,"c"), (9,10,"d")],
    #    3:[(2,7,"a")]
    #}
    create_svg(output_filename, thread_history, region_history, barrier_history)


