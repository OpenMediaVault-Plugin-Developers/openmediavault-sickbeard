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

require_once "openmediavault/object.inc";
require_once "openmediavault/config.inc";
require_once "openmediavault/error.inc";
require_once "openmediavault/util.inc";
require_once "openmediavault/rpcservice.inc";
require_once "openmediavault/notify.inc";
require_once("openmediavault/product.inc");
require_once("openmediavault/system.inc");

class OMVRpcServiceSickbeard extends OMVRpcServiceAbstract {

    public function getName() {
        return 'Sickbeard';
    }

    private function getPluginName() {
        return strtolower($this->getName());
    }

    private function getEventMessagePath() {
        $path = sprintf("org.openmediavault.service.%s", $this->getPluginName());
        return $path;
    }

    private function getEventMessagePathFork() {
        $path = sprintf("%s.forks.fork", $this->getEventMessagePath());
        return $path;
    }

    private function getXPath() {
        $path = sprintf("//services/%s", $this->getPluginName());
        return $path;
    }

    private function getXPathForks() {
        $path = sprintf("%s/forks", $this->getXPath());
        return $path;
    }


    private function getXPathFork() {
        $path = sprintf("%s/fork", $this->getXPathForks());
        return $path;
    }


    public function initialize() {
        $this->registerMethod('getSettings');
        $this->registerMethod('setSettings');
        $this->registerMethod("enumerateRepos");

        $this->registerMethod("getForks");
        $this->registerMethod("getFork");
        $this->registerMethod("setFork");
        $this->registerMethod("deleteFork");

        $this->registerMethod('doBackup');
        $this->registerMethod('doRestore');
        $this->registerMethod('getCandidates');
    }

    function getSettings($params, $context) {
        global $xmlConfig;

        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        $object = $xmlConfig->get($this->getXPath());
        if(is_null($object))
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);

        $cmd = "grep -m1 port /home/sickbeard/.sickbeard/config.ini | cut -d ' ' -f 3 2>&1";
        $this->exec($cmd, $output);
        if ($output == "")
            $output = "8081";
        $object["enable"]  = boolval($object["enable"]);
        $object["showtab"] = boolval($object["showtab"]);
        $object["port"]    = $output;

        // Remove useless fields from the result object.
        unset($object['forks']);

        return $object;
    }

    function setSettings($params, $context) {
        global $xmlConfig;

        $this->validateMethodContext($context, array("role" => OMV_ROLE_ADMINISTRATOR));

        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "enable"    : { "type" : "boolean" },
                "showtab"   : { "type" : "boolean" },
                "sb_branch" : { "type" : "string" },
                "sb_repo"   : { "type" : "string" },
                "port"      : { "type" : "integer" },
                "mntentref" : { "type" : "string" },
                "path"      : { "type" : "string" }
            }
        }');


        $xpath = sprintf("//system/fstab/mntent[uuid='%s']", $params['mntentref']);
        $deviceDir = $xmlConfig->get($xpath);
        $deviceDir = $deviceDir['dir'];


        $backupPath = "{$deviceDir}/SBbackup";
        $params['path'] = $backupPath;


        // Get existing configuration object
        $oldObject = $xmlConfig->get($this->getXPath());
        if (is_null($oldObject))
           throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);

        $object = array(
            "enable"    => array_boolval($params, "enable"),
            "showtab"   => array_boolval($params, "showtab"),
            "sb_branch" => $params['sb_branch'],
            "sb_repo"   => $params['sb_repo'],
            "port"      => $params["port"],
            "mntentref" => $params["mntentref"],
            "path"      => $params["path"],
            "forks"     => isset($oldObject['forks']) ? $oldObject['forks'] : array(),
        );

        if(false === $xmlConfig->replace($this->getXPath(), $object))
            throw new OMVException(OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED, $this->getXPath());

        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(OMV_NOTIFY_MODIFY, $this->getEventMessagePath(), $object);

        return $object;
    }

    public function getForks($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "start"     : { "type" : "integer" },
                "limit"     : { '.$GLOBALS['OMV_JSONSCHEMA_COUNTFIELD'].' },
                "sortfield" : { '.$GLOBALS['OMV_JSONSCHEMA_SORTFIELD'].' },
                "sortdir"   : { '.$GLOBALS['OMV_JSONSCHEMA_SORTDIR'].' }
            }
        }');

        // Get configuration data.
        $objects = $xmlConfig->getList( $this->getXPathFork());
        if(is_null($objects))
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);

         // Filter the result.
        return $this->applyFilter($objects, $params['start'], $params['limit'],
            $params['sortfield'], $params['sortdir']);
    }


    public function getFork($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array("role" => OMV_ROLE_ADMINISTRATOR));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "uuid" : { '.$GLOBALS['OMV_JSONSCHEMA_UUID'].' }
            }
        }');

        // Get the configuration object.
        $xpath = sprintf("%s[uuid='%s']", $this->getXPathFork(), $params['uuid']);
        $object = $xmlConfig->get($xpath);
        if(is_null($object))
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);

        return $object;
    }

    public function setFork($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "uuid"     : { '.$GLOBALS['OMV_JSONSCHEMA_UUID_UNDEFINED'].' },
                "name"     : { "type" : "string" },
                "fork"     : { "type" : "string" }
            }
        }');

        // Check if valid fork. (todo)
        if (substr( $params['fork'], 0, 4 ) !== "http" and substr( $params['fork'], 0, 3 ) !== "git")
            throw new OMVException(OMVErrorMsg::E_EXEC_MISC, "Not valid git repo");

        //if (substr( $output, 0, 6 ) === "fatal:")
        //    throw new OMVException(OMVErrorMsg::E_EXEC_MISC, "not valid git repo");
        $cmd = "git ls-remote -h ".$params['fork']." | sed 's?.*refs/heads/??'";
        $this->exec($cmd, $output);
        if (empty($output))
            throw new OMVException(OMVErrorMsg::E_EXEC_MISC, "No branches found, check url");
        //file_put_contents("/php.txt", print_r($output, true), FILE_APPEND);
        //throw new OMVException(OMVErrorMsg::E_EXEC_MISC, $output[0]);
        if (substr( $output, 0, 5 ) === "fatal")
            throw new OMVException(OMVErrorMsg::E_EXEC_MISC, "No branches found, is it a valid git repo?");

        // Prepare configuration data.
        $object = array(
            "uuid"        => ($params['uuid'] == $GLOBALS['OMV_UUID_UNDEFINED']) ? OMVUtil::uuid() : $params['uuid'],
            "name"        => $params['name'],
            "fork"        => $params['fork'],
            "branches"    => $output,
        );

        // Set the configuration data.
        $success = FALSE;
        if($params['uuid'] == $GLOBALS['OMV_UUID_UNDEFINED']) {
            // Append object to configuration.
            $success = $xmlConfig->set($this->getXPathForks(), array("fork" => $object));
        } else {
            // Update existing configuration object.
            $xpath = sprintf("%s[uuid='%s']", $this->getXPathFork(), $params['uuid']);
        }

        if(FALSE === $success)
            throw new OMVException(OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED);

        // Notify configuration changes.
        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(($params['uuid'] == $GLOBALS['OMV_UUID_UNDEFINED']) ?
            OMV_NOTIFY_CREATE : OMV_NOTIFY_MODIFY,
            $this->getEventMessagePathFork(), $object);

        // Return the configuration object.
        return $object;
    }

    public function deleteFork($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "uuid" : { '.$GLOBALS['OMV_JSONSCHEMA_UUID'].' }
            }
        }');

        // Delete the configuration object.
        $xpath = sprintf("%s[uuid='%s']", $this->getXPathFork(), $params['uuid']);
        return $this->deleteConfigObjectByPath($xpath, $this->getEventMessagePathFork());

    }
	
	public function getCandidates($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Get a list of mount points, except bind mounts.
        $xpath = "//system/fstab/mntent[not(contains(opts,'bind'))]";
        $objects = $xmlConfig->getList($xpath);
        if(is_null($objects)) {
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);
        }

        // Get the filesystem details for each mount point.
        $result = array();
        foreach($objects as $objectk => $objectv) {
            $fs = new OMVFilesystem($objectv['fsname']);
            if(!$fs->exists())
                continue;

            // Skip the filesystems that are not mounted at the moment.
            if(FALSE === $fs->isMounted())
                continue;

            // Get some filesystem details.
            $devicefile = $fs->getDeviceFile();
            $label = $fs->getLabel();

            // Prepare result object.
            $object = array(
                "uuid"        => $objectv['uuid'],
                "devicefile"  => $devicefile,
                "description" => !empty($label) ? $label : $devicefile
            );

            // Get some more filesystem details if possible.
            if(FALSE !== ($fsStats = $fs->getStatistics())) {
                $object['description'] = sprintf(
                    gettext("%s (%s available)"), $object['description'],
                    binary_format($fsStats['available']));
            }

            $result[] = $object;
        }
        return $result;
    }
	
    public function doBackup($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Create a background process.
        $bgStatusFilename = $this->createBgProcStatus();
        $pid = $this->fork();
        if($pid > 0) { // Parent process.
            $this->initializeBgProcStatus($bgStatusFilename, $pid);
            return $bgStatusFilename;
        }

        // Child process.
        try {
            $bgOutputFilename = $this->createBgProcOutput();
            $this->updateBgProcStatus($bgStatusFilename, "outputfilename", $bgOutputFilename);

            $path = $xmlConfig->get("//services/sickbeard/path");

            $cmd = sprintf("tar -zcvf %s /home/sickbeard/.sickbeard 2>&1", $path);
            $this->exec($cmd, $output, $bgOutputFilename);
            $this->finalizeBgProcStatus($bgStatusFilename, $output);
            exit(0);
        } catch(Exception $e) {
            $this->finalizeBgProcStatus($bgStatusFilename, "", $e);
            exit(1);
        }
    }
	
    public function doRestore($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Get the configuration object.
        $path = $xmlConfig->get("//services/sickbeard/path");
        $dirPath = "/home/sickbeard/.sickbeard";
        if(is_null($path)) {
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);
        }

        // Check if backup is present.
        if (file_exists($path)) {
            // Create a background process.
            $bgStatusFilename = $this->createBgProcStatus();
            $pid = $this->fork();
            if($pid > 0) { // Parent process.
                $this->initializeBgProcStatus($bgStatusFilename, $pid);
                return $bgStatusFilename;
            }

            // Delete current folder
            system("rm -rf ".escapeshellarg($dirPath));


            //if (substr($dirPath, strlen($dirPath) - 1, 1) != '/') {
            //    $dirPath .= '/';
            //}

            //$files = glob($dirPath . '*', GLOB_MARK);
            //foreach ($files as $file) {
            //    if (is_dir($file)) {
            //        self::deleteDir($file);
            //    } else {
            //        unlink($file);
            //    }
            //}
            //rmdir($dirPath);

            // Child process.
            try {
                $bgOutputFilename = $this->createBgProcOutput();
                $this->updateBgProcStatus($bgStatusFilename, "outputfilename", $bgOutputFilename);

                $cmd = sprintf("tar -zxvf %s -C / 2>&1", $path);

                $this->exec($cmd, $output, $bgOutputFilename);
                $this->finalizeBgProcStatus($bgStatusFilename, $output);
                exit(0);
            } catch(Exception $e) {
                $this->finalizeBgProcStatus($bgStatusFilename, "", $e);
                exit(1);
            }			
        } else {
            throw new OMVException(OMVErrorMsg::E_EXEC_MISC, "No backup found!");
        }
    }


    public function enumerateRepos($params, $context)
    {
        global $xmlConfig;
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Get configuration data.
        $objects = $xmlConfig->getList( $this->getXPathFork());
        if(is_null($objects))
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);

        //file_put_contents("/php.txt", print_r($objects, true), FILE_APPEND);

        $repo = array(
            "0" => array(
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
            )
        );

        $repos = array_merge($repo, $objects);
        //file_put_contents("/php.txt", print_r($repos, true), FILE_APPEND);
        return $repos;
    }
}

// Register the RPC service.
$rpcServiceMgr = &OMVRpcServiceMgr::getInstance();
$rpcServiceMgr->registerService(new OMVRpcServiceSickbeard());