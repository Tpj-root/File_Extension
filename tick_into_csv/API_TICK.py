import json
import os
import csv
import time
import requests
import websocket


# ============================================================
# CONFIGURATION
# ============================================================

SYMBOL = "frxXAUUSD"

# Your requested starting timestamp.
START_EPOCH = 1790922600

# One complete day.
DAY_SECONDS = 86400

# Last timestamp belonging to the day.
END_EPOCH = START_EPOCH + DAY_SECONDS - 1

# Deriv request limit we are using.
REQUEST_COUNT = 1000

# Output directory.
OUTPUT_DIR = "tick_history"

# Raw JSON responses.
JSON_DIR = os.path.join(
    OUTPUT_DIR,
    "json"
)

# Final CSV.
CSV_FILE = os.path.join(
    OUTPUT_DIR,
    f"{SYMBOL}_{START_EPOCH}.csv"
)


# ============================================================
# CREATE DIRECTORIES
# ============================================================

os.makedirs(
    JSON_DIR,
    exist_ok=True
)


# ============================================================
# READ CREDENTIALS FROM ENVIRONMENT
# ============================================================

APP_ID = os.environ["DERIV_APP_ID"]
TOKEN = os.environ["DERIV_API_TOKEN"]

# ALWAYS DEMO
ACCOUNT_ID = os.environ["DERIV_ACCOUNT_ID"]

# REAL
# ACCOUNT_ID = os.environ["DERIV_ACCOUNT_RID"]


# ============================================================
# DERIV OTP API
# ============================================================

API_URL = (
    f"https://api.derivws.com"
    f"/trading/v1/options/accounts/{ACCOUNT_ID}/otp"
)

headers = {
    "Deriv-App-ID": APP_ID,
    "Authorization": f"Bearer {TOKEN}",
}


# ============================================================
# DISPLAY INFORMATION
# ============================================================

print()
print("==========================================")
print("       DERIV FULL DAY TICK COLLECTOR")
print("==========================================")
print("Symbol      :", SYMBOL)
print("Start epoch :", START_EPOCH)
print("End epoch   :", END_EPOCH)
print("Duration    :", DAY_SECONDS, "seconds")
print("==========================================")
print()


# ============================================================
# GET WEBSOCKET URL
# ============================================================

print("Getting demo WebSocket URL...")


response = requests.post(
    API_URL,
    headers=headers
)


print("HTTP:", response.status_code)


data = response.json()


if response.status_code != 200:

    print(
        json.dumps(
            data,
            indent=2
        )
    )

    exit(1)


ws_url = data["data"]["url"]


# ============================================================
# CONNECT
# ============================================================

print("Connecting to DEMO account...")
print("Account:", ACCOUNT_ID)


ws = websocket.create_connection(
    ws_url,
    timeout=30
)


print("Connected!")
print()


# ============================================================
# CHECK BALANCE
# ============================================================

ws.send(
    json.dumps({
        "balance": 1
    })
)


result = json.loads(
    ws.recv()
)


if "error" in result:

    print(
        "ERROR:",
        result["error"]
    )

    ws.close()

    exit(1)


balance = result["balance"]


print("==============================")
print("       DERIV DEMO")
print("==============================")
print("Account :", ACCOUNT_ID)
print("Balance :", balance["balance"])
print("Currency:", balance["currency"])
print("==============================")
print()


# ============================================================
# ALL TICKS
# ============================================================

all_ticks = []


# ============================================================
# BACKWARD COLLECTION
# ============================================================
#
# We start at END_EPOCH.
#
# Each request asks for up to 1000 ticks ending at
# current_end.
#
# Then we move current_end backwards using the OLDEST
# timestamp returned by that request.
#
# Example:
#
# Request 1:
#
#     END
#      |
#      | <--- 1000 ticks
#      |
#   oldest_1
#
# Request 2:
#
#   oldest_1 - 1
#      |
#      | <--- 1000 ticks
#      |
#   oldest_2
#
# etc.
# ============================================================

current_end = END_EPOCH

request_number = 0


while current_end >= START_EPOCH:

    request_number += 1


    print("------------------------------------------")
    print("Request :", request_number)
    print("Start   :", START_EPOCH)
    print("End     :", current_end)
    print("------------------------------------------")


    # --------------------------------------------------------
    # Request historical ticks.
    # --------------------------------------------------------

    request = {
        "ticks_history": SYMBOL,
        "adjust_start_time": 1,
        "count": REQUEST_COUNT,
        "start": START_EPOCH,
        "end": current_end,
        "style": "ticks"
    }


    # --------------------------------------------------------
    # Send request.
    # --------------------------------------------------------

    ws.send(
        json.dumps(request)
    )


    # --------------------------------------------------------
    # Receive response.
    # --------------------------------------------------------

    raw_response = ws.recv()


    result = json.loads(
        raw_response
    )


    # --------------------------------------------------------
    # Check API error.
    # --------------------------------------------------------

    if "error" in result:

        print()
        print("DERIV ERROR:")

        print(
            json.dumps(
                result,
                indent=2
            )
        )

        ws.close()

        exit(1)


    # ========================================================
    # SAVE COMPLETE RAW JSON RESPONSE
    # ========================================================

    json_file = os.path.join(
        JSON_DIR,
        f"response_{request_number:06d}.json"
    )


    with open(
        json_file,
        "w",
        encoding="utf-8"
    ) as f:

        json.dump(
            result,
            f,
            indent=2
        )


    print(
        "JSON saved:",
        json_file
    )


    # ========================================================
    # GET HISTORY
    # ========================================================

    if "history" in result:

        history = result["history"]

    else:

        history = result


    prices = history.get(
        "prices",
        []
    )

    times = history.get(
        "times",
        []
    )


    # --------------------------------------------------------
    # No data.
    # --------------------------------------------------------

    if not prices or not times:

        print()
        print("No more tick data.")

        break


    # --------------------------------------------------------
    # Make sure both arrays have same length.
    # --------------------------------------------------------

    number_of_ticks = min(
        len(prices),
        len(times)
    )


    print(
        "Ticks received:",
        number_of_ticks
    )


    # ========================================================
    # STORE TICKS
    # ========================================================

    for i in range(number_of_ticks):

        timestamp = int(
            times[i]
        )

        price = float(
            prices[i]
        )


        # Only keep requested day.

        if (
            START_EPOCH
            <= timestamp
            <= END_EPOCH
        ):

            all_ticks.append(
                (
                    timestamp,
                    price
                )
            )


    # ========================================================
    # FIND OLDEST TIMESTAMP
    # ========================================================
    #
    # IMPORTANT:
    #
    # We need the OLDEST timestamp, not the newest timestamp.
    #
    # min(times) gives us the earliest tick returned.
    # ========================================================

    oldest_timestamp = min(
        int(t)
        for t in times
    )


    newest_timestamp = max(
        int(t)
        for t in times
    )


    print(
        "Oldest timestamp:",
        oldest_timestamp
    )

    print(
        "Newest timestamp:",
        newest_timestamp
    )

    print(
        "Total collected:",
        len(all_ticks)
    )


    # ========================================================
    # STOP CONDITIONS
    # ========================================================

    if oldest_timestamp <= START_EPOCH:

        print()
        print("Reached beginning of requested day.")

        break


    # --------------------------------------------------------
    # Move backwards.
    #
    # -1 prevents requesting the same tick again.
    # --------------------------------------------------------

    next_end = oldest_timestamp - 1


    # Safety check.
    if next_end >= current_end:

        print()
        print("ERROR: Timestamp did not move backwards.")

        break


    current_end = next_end


    # Small pause between requests.
    time.sleep(0.2)


# ============================================================
# CLOSE WEBSOCKET
# ============================================================

ws.close()


print()
print("WebSocket closed.")
print()


# ============================================================
# REMOVE DUPLICATES
# ============================================================

unique_ticks = list(
    dict.fromkeys(
        all_ticks
    )
)


# ============================================================
# SORT CHRONOLOGICALLY
# ============================================================

unique_ticks.sort(
    key=lambda x: x[0]
)


# ============================================================
# WRITE CSV
# ============================================================

print("Writing final CSV...")


with open(
    CSV_FILE,
    "w",
    newline="",
    encoding="utf-8"
) as f:

    writer = csv.writer(f)


    # Header

    writer.writerow([
        "times",
        "prices"
    ])


    # Data

    for timestamp, price in unique_ticks:

        writer.writerow([
            timestamp,
            price
        ])


# ============================================================
# FINAL RESULT
# ============================================================

print()
print("==========================================")
print("          COLLECTION COMPLETE")
print("==========================================")
print("Symbol          :", SYMBOL)
print("Start epoch     :", START_EPOCH)
print("End epoch       :", END_EPOCH)
print("Requests        :", request_number)
print("Ticks collected :", len(unique_ticks))
print()
print("JSON directory  :", JSON_DIR)
print("CSV file        :", CSV_FILE)
print("==========================================")
print()