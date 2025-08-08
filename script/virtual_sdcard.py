    def getXYZE(self, file_path, file_position):
        result = {"X": 0, "Y": 0, "Z": 0, "E": 0}
        try:
            import io
            with io.open(file_path, "r", encoding="utf-8") as f:
                f.seek(file_position)
                while True:
                    line = f.readline()
                    line_list = line.split(" ")
                    if not result["E"] and "E" in line:
                        for obj in line_list:
                            if obj.startswith("E"):
                                try:
                                    ret = obj[1:].split("\r")[0]
                                    ret = ret.split("\n")[0]
                                    if ret.startswith("."):
                                        logging.info("power_loss getXYZE E:%s" % obj)
                                        result["E"] = float(("0"+ret.strip(" ")))
                                    else:
                                        logging.info("power_loss getXYZE E:%s" % obj)
                                        result["E"] = float(ret.strip(" "))
                                except Exception as err:
                                    logging.exception(err)
                    if not result["X"]:
                        for obj in line_list:
                            if obj.startswith("X"):
                                try:
                                    logging.info("power_loss getXYZE X:%s" % obj)
                                    result["X"] = float(obj.split("\r")[0][1:])
                                except Exception as err:
                                    logging.exception(err)
                    if not result["Y"]:
                        for obj in line_list:
                            if obj.startswith("Y"):
                                try:
                                    logging.info("power_loss getXYZE Y:%s" % obj)
                                    result["Y"] = float(obj.split("\r")[0][1:])
                                except Exception as err:
                                    logging.exception(err)
                    if not result["Z"] and "Z" in line:
                        for obj in line_list:
                            if obj.startswith("Z"):
                                try:
                                    logging.info("power_loss getXYZE Z:%s" % obj)
                                    result["Z"] = float(obj.split("\r")[0][1:])
                                except Exception as err:
                                    logging.exception(err)
                    if result["X"] and result["Y"] and result["Z"] and result["E"]:
                        logging.info("get XYZE:%s" % str(result))
                        logging.info("power_loss get XYZE:%s" % str(result))
                        break
                    self.reactor.pause(self.reactor.monotonic() + .001)
        except UnicodeDecodeError as err:
            logging.exception(err)
            # UnicodeDecodeError 'utf-8' codec can't decode byte 0xff in postion 5278: invalid start byte
            err_msg = '{"code": "key572", "msg": "File UnicodeDecodeError"}'
            self.gcode.respond_info(err_msg)
            raise self.printer.command_error(err_msg)
        except Exception as err:
            logging.exception(err)
        return result