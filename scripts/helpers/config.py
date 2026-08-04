import json
from functools import lru_cache

from helpers.root_path import ROOT_PATH


@lru_cache
def load_config(config_path):
    try:
        with open(ROOT_PATH / config_path) as f:
            return json.load(f)
    except FileNotFoundError as e:
        raise Exception(f"Error: Could not find config_path='{config_path}'") from e
