"""
ptz_client.py  –  HTTP client for PTZControl REST API
pip install requests
"""
import requests

class PTZClient:
    def __init__(self, host="localhost", port=8080):
        self._base = f"http://{host}:{port}"

    def _get(self, path, **params):
        r = requests.get(self._base + path, params=params, timeout=5)
        r.raise_for_status()
        return r.json()

    def _post(self, path, **params):
        r = requests.post(self._base + path, params=params, timeout=5)
        r.raise_for_status()
        return r.json()

    def status(self):
        """Returns {'cameras': N}"""
        return self._get("/status")

    def get_zoom(self, camera=-1):
        return self._get("/zoom", camera=camera)["zoom"]

    def pan(self, direction, camera=-1):
        """direction: positive=right, negative=left, 0=stop"""
        return self._post("/pan", camera=camera, direction=direction)["success"]

    def tilt(self, direction, camera=-1):
        """direction: positive=up, negative=down, 0=stop"""
        return self._post("/tilt", camera=camera, direction=direction)["success"]

    def zoom(self, direction, camera=-1):
        """direction: positive=in, negative=out"""
        return self._post("/zoom", camera=camera, direction=direction)

    def home(self, camera=-1):
        return self._post("/home", camera=camera)["success"]

    def recall_preset(self, preset, camera=-1):
        """preset: 0-7"""
        return self._post("/preset/recall", camera=camera, preset=preset)["success"]

    def save_preset(self, preset, camera=-1):
        """preset: 0-7"""
        return self._post("/preset/save", camera=camera, preset=preset)["success"]


if __name__ == "__main__":
    import time
    cam = PTZClient()
    print(cam.status())
    print("zoom:", cam.get_zoom())
    cam.pan(1);  time.sleep(0.4);  cam.pan(0)
    cam.tilt(1); time.sleep(0.4);  cam.tilt(0)
    cam.home()
    print("done")
