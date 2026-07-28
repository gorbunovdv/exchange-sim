import json


def load_config(config_path):
    try:
        with open(config_path) as f:
            return json.load(f)
    except FileNotFoundError as e:
        raise Exception(f"Error: Could not find config_path='{config_path}'") from e
