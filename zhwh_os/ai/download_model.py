#!/usr/bin/env python3
"""
Download IBM Granite 3.0 Nano from Hugging Face
Model: ibm-granite/granite-3.0-2b-instruct
GGUF quantized version: bartowski/granite-3.0-2b-instruct-GGUF
"""
import os
from huggingface_hub import snapshot_download, hf_hub_download

MODEL_ID = "ibm-granite/granite-3.0-2b-instruct"
GGUF_ID  = "bartowski/granite-3.0-2b-instruct-GGUF"
LOCAL_DIR = os.path.dirname(os.path.abspath(__file__))

def download_gguf():
    """Download Q4_K_M quantized GGUF (best balance for embedded)"""
    print(f"Downloading GGUF model from {GGUF_ID}...")
    files = [
        "granite-3.0-2b-instruct-Q4_K_M.gguf",
    ]
    for f in files:
        dest = os.path.join(LOCAL_DIR, "models", f)
        if os.path.exists(dest):
            print(f"  SKIP {f} (already exists)")
            continue
        print(f"  GET {f} ...")
        hf_hub_download(repo_id=GGUF_ID, filename=f,
                        local_dir=os.path.join(LOCAL_DIR, "models"),
                        local_dir_use_symlinks=False)
    print("Done.")

if __name__ == "__main__":
    download_gguf()
