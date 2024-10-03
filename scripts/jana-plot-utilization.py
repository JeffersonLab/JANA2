
import re
import svgwrite
from datetime import datetime, timedelta
from collections import defaultdict


def parse_logfile():

    # Parse the logs and store intervals
    thread_history = defaultdict(list)  # Key: thread_id, Value: list of (start_time, end_time, processor_name)
    start_times = {}  # Key: (thread_id, processor_name), Value: start_time

    # Define a regular expression to parse the log lines
    log_pattern = re.compile(r"^(\d{2}):(\d{2}):(\d{2})\.(\d{3}) \[debug\] (\d+) (Executing|Executed) arrow (\w+)")

    with open("log.txt", "r") as log_file:
        for line in log_file:
            match = re.search(log_pattern, line.strip())
            if match:
                hours_str, mins_str, secs_str, millis_str, thread_id, action, processor_name = match.groups()

                # Convert timestamp to milliseconds
                millis = (((int(hours_str) * 60) + int(mins_str) * 60) + int(secs_str)) * 1000 + int(millis_str)

                if action == "Executing":
                    # Log the start time of the processor for the thread
                    start_times[(thread_id, processor_name)] = millis

                elif action == "Executed":
                    # Calculate the duration of the processor and store the interval
                    start_time = start_times.pop((thread_id, processor_name), None)
                    if start_time:
                        thread_history[thread_id].append((start_time, millis, processor_name))

    return thread_history


def create_svg(all_thread_history):
    # Assign colors to processors
    processor_colors = {}
    color_palette = ['#FF6347', '#4682B4', '#32CD32', '#FFD700', '#9370DB', '#FF69B4']
    color_index = 0

    # Figure out drawing coordinate system
    overall_start_time = min(start for history in all_thread_history.values() for (start,_,_) in history)
    overall_end_time = max(end for history in all_thread_history.values() for (_,end,_) in history)
    thread_count = len(all_thread_history)
    width=1000
    x_scale = width/(overall_end_time-overall_start_time)
    thread_height=40
    thread_y_padding=20
    height=thread_count * thread_height + (thread_count+1) * thread_y_padding


    # Create the SVG drawing
    dwg = svgwrite.Drawing("timeline.svg", profile='tiny', size=(width, height))
    dwg.add(dwg.rect(insert=(0,0),size=(width,height),stroke="red",fill="white"))

    # Draw a rectangle for each processor run on each thread's timeline
    y_position = thread_y_padding

    for thread_id, intervals in all_thread_history.items():
        dwg.add(dwg.rect(insert=(0,y_position),size=(1000,thread_height),stroke="lightgray",fill="lightgray"))
        for start_time, end_time, processor_name in intervals:
            # Calculate the position and width of the rectangle
            # Assign a unique color to each processor name
            if processor_name not in processor_colors:
                processor_colors[processor_name] = color_palette[color_index % len(color_palette)]
                color_index += 1

            # Draw the rectangle
            rect_start = (start_time-overall_start_time)*x_scale
            rect_width = (end_time-start_time)*x_scale
            rect = dwg.rect(insert=(rect_start, y_position), 
                             size=(rect_width, thread_height),
                             fill=processor_colors[processor_name],
                             stroke="black",
                             stroke_width=0)
            mouseover = processor_name + ": " + str(end_time-start_time) + "ms"
            rect.add(svgwrite.base.Title(mouseover))
            dwg.add(rect)

        # Move the y position for the next thread
        y_position += (thread_y_padding + thread_height)

    # Save the SVG file
    dwg.save()



if __name__ == "__main__":
    thread_history = parse_logfile()
    #thread_history = {
    #    1103:[(0,1,"a"), (2,5,"b"), (6,8,"a")],
    #    219:[(0,3,"b"), (3,6,"c"), (9,10,"d")],
    #    3:[(2,7,"a")]
    #}
    create_svg(thread_history)


