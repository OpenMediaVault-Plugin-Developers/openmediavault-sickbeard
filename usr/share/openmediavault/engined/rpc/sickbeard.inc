<?php

/**
 * Copyright (C) 2013-2014 OpenMediaVault Plugin Developers
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

require_once "openmediavault/config.inc";
require_once "openmediavault/error.inc";
require_once "openmediavault/notify.inc";
require_once "openmediavault/object.inc";
require_once "openmediavault/product.inc";
require_once "openmediavault/rpcservice.inc";
require_once "openmediavault/system.inc";
require_once "openmediavault/util.inc";

class OMVRpcServiceSickbeard extends OMVRpcServiceAbstract
{
    private $sickbeardSettingsDir = "/home/sickbeard/.sickbeard/";
    private $sickbeardSettingsFile = "/home/sickbeard/.sickbeard/config.ini";

    private function getPluginName()
    {
        return strtolower($this->getName());
    }

    private function getEventMessagePath()
    {
        return sprintf("org.openmediavault.service.%s", $this->getPluginName());
    }

    private function getXPath()
    {
        return sprintf("//services/%s", $this->getPluginName());
    }

    private function getXPathForks()
    {
        return sprintf("%s/forks", $this->getXPath());
    }

    private function getXPathFork()
    {
        return sprintf("%s/fork", $this->getXPathForks());
    }

    public function getName()
    {
        return "Sickbeard";
    }

    public function initialize()
    {
        $this->registerMethod("getSettings");
        $this->registerMethod("setSettings");
        $this->registerMethod("enumerateRepos");

        $this->registerMethod("getForks");
        $this->registerMethod("getFork");
        $this->registerMethod("setFork");
        $this->registerMethod("deleteFork");

        $this->registerMethod("doBackup");
        $this->registerMethod("doRestore");
    }

    public function getSettings($params, $context)
    {
        global $xmlConfig;

        $this->validateMethodContext($context, array(
            "role" => OMV_ROLE_ADMINISTRATOR
        ));

        $object = $xmlConfig->get($this->getXPath());

        if (is_null($object)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXPath()
            );
        }

        $port = 8081;

        // Get current port as set in sickbeards config file
        if (file_exists($this->sickbeardSettingsFile)) {
            $config = parse_ini_file(
                $this->sickbeardSettingsFile,
                true,
                INI_SCANNER_RAW
            );

            if (!empty($config["General"]["port"])) {
                $port = $config["General"]["port"];
            }

            if (!empty($config["General"]["web_port"])) {
                $port = $config["General"]["web_port"];
            }
        }

        $object["enable"]         = boolval($object["enable"]);
        $object["showtab"]        = boolval($object["showtab"]);
        $object["newinstance"]    = boolval($object["newinstance"]);
        $object["newinstenable"]  = boolval($object["newinstenable"]);
        $object["ssl"]            = boolval($object["ssl"]);
        $object["ppass"]          = boolval($object["ppass"]);
        $object["port"]           = intval($port);

        // Remove useless fields from the result object.
        unset($object["forks"]);

        return $object;
    }

    public function setSettings($params, $context)
    {
        global $xmlConfig;

        $this->validateMethodContext($context, array(
            "role" => OMV_ROLE_ADMINISTRATOR
        ));

        $this->validateMethodParams(
            $params,
            '{
                "type"       : "object",
                "properties" : {
                    "enable"    : { "type" : "boolean" },
                    "showtab"       : { "type" : "boolean" },
                    "newinstance"   : { "type" : "boolean" },
                    "newinstenable" : { "type" : "boolean" },
                    "ppass"         : { "type" : "boolean" },
                    "ssl"           : { "type" : "boolean" },
                    "branch"        : { "type" : "string" },
                    "repo"          : { "type" : "string" },
                    "port"          : { "type" : "integer" },
                    "mntentref"     : { ' . $GLOBALS['OMV_JSONSCHEMA_UUID_NONE'] . ' },
                    "path"          : { "type" : "string", "optional" : true }
                }
            }'
        );

        $xPath = sprintf("//system/fstab/mntent[uuid='%s']", $params["mntentref"]);
        $deviceDir = $xmlConfig->get($xPath);
        $deviceDir = $deviceDir["dir"];

        $backupPath = "$deviceDir/SBbackup";
        $params["path"] = $backupPath;

        // Get existing configuration object
        $oldObject = $xmlConfig->get($this->getXPath());

        if (is_null($oldObject)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXPath()
            );
        }

        // Check if ssl certs are available
        //if ($params["ssl"] == "1") {
        //     $cmd = "ls /etc/ssl/private/openmediavault-*.key";
        //     $this->exec($cmd, $output);
        //     if ($output[0] == "ls: cannot access /etc/ssl/private/openmediavault-*.key: No such file or directory") {
        //         throw new OMVException(
        //             OMVErrorMsg::E_EXEC_MISC,
        //             substr($output, 0, 5) . "No Certs were found, can not enable SSL"
        //         );
        //     }
        //}

        $object = array(
            "enable"        => array_boolval($params, "enable"),
            "showtab"       => array_boolval($params, "showtab"),
            "newinstance"   => array_boolval($params, "newinstance"),
            "newinstenable" => array_boolval($params, "newinstenable"),
            "ppass"         => array_boolval($params, "ppass"),
            "ssl"           => array_boolval($params, "ssl"),
            "branch"        => $params['branch'],
            "repo"          => $params["repo"],
            "port"          => $params["port"],
            "mntentref"     => $params["mntentref"],
            "path"          => $params["path"],
            "forks"         => isset($oldObject['forks']) ? $oldObject['forks'] : array(),
        );

        if (false === $xmlConfig->replace($this->getXPath(), $object)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED,
                $this->getXPath()
            );
        }

        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(
            OMV_NOTIFY_MODIFY,
            $this->getEventMessagePath(),
            $object
        );

        return $object;
    }

    public function getForks($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array("role" => OMV_ROLE_ADMINISTRATOR));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams(
            $params,
            '{
                "type"       : "object",
                "properties" : {
                    "start"     : { "type" : "integer" },
                    "limit"     : { ' . $GLOBALS["OMV_JSONSCHEMA_COUNTFIELD"] . ' },
                    "sortfield" : { ' . $GLOBALS["OMV_JSONSCHEMA_SORTFIELD"] . ' },
                    "sortdir"   : { ' . $GLOBALS["OMV_JSONSCHEMA_SORTDIR"] . ' }
                }
            }'
        );

        // Get configuration data.
        $objects = $xmlConfig->getList($this->getXPathFork());

        if (is_null($objects)) {
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $this->getXPathFork());
        }

        // Filter the result.
        return $this->applyFilter(
            $objects,
            $params["start"],
            $params["limit"],
            $params["sortfield"],
            $params["sortdir"]
        );
    }

    public function getFork($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array(
            "role" => OMV_ROLE_ADMINISTRATOR
        ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams(
            $params,
            '{
                "type"       : "object",
                "properties" : {
                    "uuid" : { ' . $GLOBALS["OMV_JSONSCHEMA_UUID"] . ' }
                }
            }'
        );

        // Get the configuration object.
        $xPath = sprintf("%s[uuid='%s']", $this->getXPathFork(), $params["uuid"]);
        $object = $xmlConfig->get($xPath);

        if (is_null($object)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $xPath
            );
        }

        return $object;
    }

    public function setFork($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array(
            "role" => OMV_ROLE_ADMINISTRATOR
        ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams(
            $params,
            '{
                "type"       : "object",
                "properties" : {
                    "uuid" : { ' . $GLOBALS["OMV_JSONSCHEMA_UUID_UNDEFINED"] . ' },
                    "name" : { "type" : "string" },
                    "fork" : { "type" : "string" }
                }
            }'
        );

        // Check if valid fork. A valid fork should start with
        // either http(s):// or git://
        if (substr($params["fork"], 0, 4) !== "http" &&
            substr($params["fork"], 0, 3) !== "git"
        ) {
            throw new OMVException(
                OMVErrorMsg::E_EXEC_MISC,
                "Not valid git repo"
            );
        }

        // Query repo for a list of branches.
        $cmd = "git ls-remote -h "
             . $params["fork"]
             . " | sed 's?.*refs/heads/??'";

        $this->exec($cmd, $output);

        if (empty($output)) {
            throw new OMVException(
                OMVErrorMsg::E_EXEC_MISC,
                "No branches found, check url"
            );
        }

        if (substr($output, 0, 5) === "fatal") {
            throw new OMVException(
                OMVErrorMsg::E_EXEC_MISC,
                "No branches found, is it a valid git repo?"
            );
        }

        // Prepare configuration data and add all branches found
        $object = array(
            "uuid"        => ($params["uuid"] == $GLOBALS["OMV_UUID_UNDEFINED"])
                ? OMVUtil::uuid() : $params["uuid"],
            "name"        => $params["name"],
            "fork"        => $params["fork"],
            "branches"    => $output,
        );

        // Set the configuration data.
        $success = false;

        if ($params["uuid"] == $GLOBALS["OMV_UUID_UNDEFINED"]) {
            // Append object to configuration.
            $success = $xmlConfig->set(
                $this->getXPathForks(),
                array("fork" => $object)
            );
        } else {
            // Update existing configuration object.
            $xPath = sprintf(
                "%s[uuid='%s']",
                $this->getXPathFork(),
                $params["uuid"]
            );
        }

        if ($success === false) {
            throw new OMVException(OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED);
        }

        // Return the configuration object.
        return $object;
    }

    public function deleteFork($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array(
            "role" => OMV_ROLE_ADMINISTRATOR
        ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams(
            $params,
            '{
                "type"       : "object",
                "properties" : {
                    "uuid" : { ' . $GLOBALS["OMV_JSONSCHEMA_UUID"] . ' }
                }
            }'
        );

        // Delete the configuration object.
        $xPath = sprintf("%s[uuid='%s']", $this->getXPathFork(), $params["uuid"]);

        return $this->deleteConfigObjectByPath($xPath);
    }

    public function doBackup($params, $context)
    {
        global $xmlConfig;

        // Sickbeard needs to have run once to generate its
        // settings. Check if this is true
        if (!file_exists($this->sickbeardSettingsFile)) {
            throw new OMVException(
                OMVErrorMsg::E_EXEC_MISC,
                "No settings folder found!"
            );
        }

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array(
            "role" => OMV_ROLE_ADMINISTRATOR
        ));

        // Create a background process.
        $bgStatusFilename = $this->createBgProcStatus();
        $pid = $this->fork();

        // Parent process.
        if ($pid > 0) {
            $this->initializeBgProcStatus($bgStatusFilename, $pid);
            return $bgStatusFilename;
        }

        // Child process.
        try {
            $bgOutputFilename = $this->createBgProcOutput();
            $this->updateBgProcStatus(
                $bgStatusFilename,
                "outputfilename",
                $bgOutputFilename
            );

            // Get the configuration object.
            $xPath = sprintf("%s/path", $this->getXPath());
            $backupPath = $xmlConfig->get($xPath);

            $cmd = sprintf(
                "tar -zcvPf %s %s 2>&1",
                $backupPath,
                $this->sickbeardSettingsDir
            );

            $this->exec($cmd, $output, $bgOutputFilename);
            $this->finalizeBgProcStatus($bgStatusFilename, $output);
            exit(0);
        } catch (Exception $e) {
            $this->finalizeBgProcStatus($bgStatusFilename, "", $e);
            exit(1);
        }
    }

    public function doRestore($params, $context)
    {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array(
            "role" => OMV_ROLE_ADMINISTRATOR
        ));

        // Get the configuration object.
        $xPath = sprintf("%s/path", $this->getXPath());
        $backupPath = $xmlConfig->get($xPath);

        if (is_null($backupPath)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $xPath
            );
        }

        // Check if backup is present.
        if (file_exists($backupPath)) {
            // Create a background process.
            $bgStatusFilename = $this->createBgProcStatus();
            $pid = $this->fork();

            // Parent process.
            if ($pid > 0) {
                $this->initializeBgProcStatus($bgStatusFilename, $pid);
                return $bgStatusFilename;
            }

            // Delete current settings folder
            rmdir($this->sickbeardSettingsDir);

            // Get the Sickbeard module
            $moduleMgr = &OMVModuleMgr::getInstance();
            $sickbeardModule = $moduleMgr->getModule($this->getName());

            // Child process.
            try {
                $sickbeardModule->stopService();
                $bgOutputFilename = $this->createBgProcOutput();
                $this->updateBgProcStatus(
                    $bgStatusFilename,
                    "outputfilename",
                    $bgOutputFilename
                );

                $cmd = sprintf("tar -zxvf %s -C / 2>&1", $backupPath);

                $this->exec($cmd, $output, $bgOutputFilename);
                $this->finalizeBgProcStatus($bgStatusFilename, $output);
                $sickbeardModule->startService();
                exit(0);
            } catch (Exception $e) {
                $this->finalizeBgProcStatus($bgStatusFilename, "", $e);
                $sickbeardModule->startService();
                exit(1);
            }
        } else {
            throw new OMVException(OMVErrorMsg::E_EXEC_MISC, "No backup found!");
        }
    }

    public function enumerateRepos($params, $context)
    {
        global $xmlConfig;

        $this->validateMethodContext($context, array(
            "role" => OMV_ROLE_ADMINISTRATOR
        ));

        // Get configuration data.
        $userRepos = $xmlConfig->getList($this->getXPathFork());

        if (is_null($userRepos)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXPathFork()
            );
        }

        // Add default repos and their branches. Do it here so users can't
        // delete them.
        $defaultRepos = array(
            array(
                "uuid" => "11111111-2222-3333-4444-555555555555",
                "name" => "Main Branch",
                "fork" => "https://github.com/midgetspy/Sick-Beard.git",
                "branches" => array(
                    "1080i",
                    "anime",
                    "custom_torrents",
                    "development",
                    "katph",
                    "master",
                    "multi_snatch",
                    "naming_error_info",
                    "storm",
                    "windows_binaries",
                    "xmpp"
                )
            ),
        );

        $repos = array_merge($defaultRepos, $userRepos);

        return $repos;
    }
}

// Register the RPC service.
$rpcServiceMgr = &OMVRpcServiceMgr::getInstance();
$rpcServiceMgr->registerService(new OMVRpcServiceSickbeard());
