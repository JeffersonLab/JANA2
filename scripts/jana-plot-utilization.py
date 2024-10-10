
import re
import svgwrite
from datetime import datetime, timedelta
from collections import defaultdict


def parse_logfile():

    # Parse the logs and store intervals
    thread_history = defaultdict(list)  # Key: thread_id, Value: list of (start_time, end_time, processor_name, event_nr, result)
    barrier_history = [] # [(released_timestamp, finished_timestamp)]

    start_times = {}  # Key: (thread_id, processor_name), Value: start_time

    # Define a regular expression to parse the log lines
    source_start_pattern = re.compile(r"^(\d{2}):(\d{2}):(\d{2})\.(\d{3}) \[debug\] (\d+) Executing arrow (\w+)$")
    source_finish_noemit_pattern = re.compile(r"^(\d{2}):(\d{2}):(\d{2})\.(\d{3}) \[debug\] (\d+) Executed arrow (\w+) with result (\w+)$")
    source_finish_emit_pattern = re.compile(r"^(\d{2}):(\d{2}):(\d{2})\.(\d{3}) \[debug\] (\d+) Executed arrow (\w+) with result (\w+), emitting event# (\d+)$")
    source_finish_pending_pattern = re.compile(r"^(\d{2}):(\d{2}):(\d{2})\.(\d{3}) \[debug\] (\d+) Executed arrow (\w+) with result (\w+), holding back barrier event# (\d+)$")
    processor_start_pattern = re.compile(r"^(\d{2}):(\d{2}):(\d{2})\.(\d{3}) \[debug\] (\d+) Executing arrow (\w+) for event# (\d+)$")
    processor_finish_pattern = re.compile(r"^(\d{2}):(\d{2}):(\d{2})\.(\d{3}) \[debug\] (\d+) Executed arrow (\w+) for event# (\d+)$")
    barrier_inflight_pattern = re.compile(r"^(\d{2}):(\d{2}):(\d{2})\.(\d{3}) \[debug\] (\d+) JEventSourceArrow: Barrier event is in-flight$")
    barrier_finished_pattern = re.compile(r"^(\d{2}):(\d{2}):(\d{2})\.(\d{3}) \[debug\] (\d+) JEventSourceArrow: Barrier event finished, returning to normal operation$")

    with open("log.txt", "r") as log_file:
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

    return (thread_history, barrier_history)



def create_svg(all_thread_history, barrier_history):
    # Assign colors to processors
    processor_colors = {}
    color_palette = ['#004E64', '#00A5CF', '#9FFFCB', '#25A18E', '#7AE582', '#FF69B4']
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
    dwg = svgwrite.Drawing("timeline.svg", profile='tiny', size=(width, height))
    #dwg.add(dwg.rect(insert=(0,0),size=(width,height),stroke="red",fill="white"))

    # Draw a rectangle for each processor run on each thread's timeline
    y_position = thread_y_padding

    for barrier_start,barrier_end in barrier_history:
        rect_start = (barrier_start-overall_start_time)*x_scale
        if (barrier_end == barrier_start): 
            rect_width=1
        else:
            rect_width = (barrier_end-barrier_start)*x_scale

        rect = dwg.rect(insert=(rect_start, 0), 
                            size=(rect_width, height),
                            fill="red",
                            stroke="none",
                            stroke_width=1)
        dwg.add(rect)

    for thread_id, intervals in all_thread_history.items():
        dwg.add(dwg.rect(insert=(0,y_position),size=(1000,thread_height),stroke="lightgray",fill="lightgray"))
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


            rect = dwg.rect(insert=(rect_start, y_position), 
                             size=(rect_width, thread_height),
                             fill=processor_colors[processor_name],
                             stroke=rect_stroke_color,
                             stroke_width=1)
            mouseover = "Arrow: " + processor_name + "\nEvent nr: " + str(event_nr) + "\nResult: " + result + "\nTime: "+ str(end_time-start_time) + "ms"
            rect.add(svgwrite.base.Title(mouseover))
            dwg.add(rect)
            if (event_nr is not None):
                text = dwg.text(str(event_nr), insert=(rect_start+1, y_position+thread_height-1), fill="white", font_size=8)
                dwg.add(text)

        # Move the y position for the next thread
        y_position += (thread_y_padding + thread_height)


    # Save the SVG file
    dwg.save()



if __name__ == "__main__":
    thread_history,barrier_history = parse_logfile()
    #thread_history = {
    #    1103:[(0,1,"a"), (2,5,"b"), (6,8,"a")],
    #    219:[(0,3,"b"), (3,6,"c"), (9,10,"d")],
    #    3:[(2,7,"a")]
    #}
    create_svg(thread_history, barrier_history)


