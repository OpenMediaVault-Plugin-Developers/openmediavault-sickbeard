<?php

/**
 * Copyright (C) 2010-2012 Ian Moore <imooreyahoo@gmail.com>
 * Copyright (C) 2013-2015 OpenMediaVault Plugin Developers.
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
 //file_put_contents('filename.txt', print_r($b, true));

namespace OMV\Engined\Rpc;

use DateTime;
use Exception;
use OMV\Config\Database;
use OMV\Engine\Notify\Dispatcher;
use OMV\Rpc\ServiceAbstract;
use OMV\Rpc\ServiceManager as RpcServiceManager;
use OMV\System\Process;
use SplFileObject;

class Sickbeard extends ServiceAbstract
{
    private $database;

    private $dataModelPath = 'conf.service.sickbeard';

    private $forkDataModelPath = 'conf.service.sickbeard.fork';

    private $settings1DataModelPath = 'conf.service.sickbeard.setsettings1';

    private $settings2DataModelPath = 'conf.service.sickbeard.setsettings2';

    private $dispatcher;

    private $eventMessagePath = 'org.openmediavault.conf.services.sickbeard';

    public function __construct(Database $database = null, Dispatcher $dispatcher = null)
    {
        $this->database = $database ?: Database::getInstance();
        $this->dispatcher = $dispatcher ?: Dispatcher::getInstance();
    }

    public function getName()
    {
        return 'Sickbeard';
    }

    public function initialize()
    {

        $this->registerMethod("getInstance");
        $this->registerMethod("setInstance");

        $this->registerMethod("getSettings1");
        $this->registerMethod("setSettings1");
        $this->registerMethod("getSettings2");
        $this->registerMethod("setSettings2");

        $this->registerMethod("enumerateRepos");

        $this->registerMethod("getForks");
        $this->registerMethod("getFork");
        $this->registerMethod("setFork");
        $this->registerMethod("deleteFork");

        $this->registerMethod("doBackup1");
        $this->registerMethod("doRestore1");
        $this->registerMethod("doBackup2");
        $this->registerMethod("doRestore2");
        $this->registerMethod("doDelete");
    }

    public function getInstance($params, $context)
    {
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);

        $object = $this->database->get($this->dataModelPath);

        $object->remove('instances');
        $object->remove('forks');

        return $object->getAssoc();
    }

    public function setInstance($params, $context)
    {
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        $this->validateMethodParams($params, 'rpc.sickbeard.setinstance');

        $object = $this->database->get($this->dataModelPath);
        $object->setAssoc($params);

        $this->database->set($object);
        $this->dispatcher->notify(OMV_NOTIFY_MODIFY, $this->eventMessagePath, $object);

        return $object->getAssoc();
    }

    public function getSettings1($params, $context)
    {
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);

        $object = $this->database->get($this->settings1DataModelPath);

        $port = 8081;

        // Get current port as set in sickbeards config file.
        if (file_exists($object->get('db-folder')."/config.ini")) {
            $config = parse_ini_file(
                $object->get('db-folder')."/config.ini",
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

        $object->add('port', 'integer');
        $object->set('port', intval($port));

        return $object->getAssoc();
    }

    public function setSettings1($params, $context)
    {
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);

        if (empty($params['mntentref']) || $params['mntentref'] == "Select a volume or leave blank for default.") {
            $params['mntentref'] = "Select a volume or leave blank for default.";
            $this->validateMethodParams($params, 'rpc.sickbeard.setsettings1');
            $params['db-folder'] = "/var/opt/sickbeard";
        } else {
            $this->validateMethodParams($params, 'rpc.sickbeard.setsettings1');
            $db = \OMV\Config\Database::getInstance();
            $deviceDir = $db->getAssoc("conf.system.filesystem.mountpoint", $params['mntentref']);
            $params['db-folder'] = sprintf("{$deviceDir['dir']}/%s", $this->getName());
        }

        // Get existing configuration object
        $oldObject = $this->database->get($this->settings1DataModelPath);

        // Run checks to verify sellected settings
        if ($params['ssl'] == 1) {
            if ($params['ppass'] == 1) {
                throw new Exception('You can not enable both SSL and Proxy Pass.');
            }
            $db = \OMV\Config\Database::getInstance();
            $getInfo = $db->getAssoc("conf.webadmin", $params['enablessl']);
            $xSsl = $getInfo['enablessl'];
            if ($xSsl == "1") {
                throw new Exception('You have SSL enabled for the main UI, Do NOT enable it here as well.');
            }

            $getInfo = $db->getAssoc('conf.system.certificate.ssl');

            if (! array_key_exists('uuid', $getInfo[0])) {
                throw new Exception('You have not generated any SSL certificates.');
            }
        }

        if ($params['showtab'] == 1 && $params['ppass'] != 1) {
            if (! file_exists('/var/www/openmediavault/js/omv/module/admin/service/media/Media.js')) {
                throw new Exception('Install links and add an entry or enable Proxy Pass to use the web tab.');
            }
        }

        if ($params['db-folder'] != $oldObject->get('db-folder') && $oldObject->get('db-folder') != '') {
            $cmd = 'mv '.$oldObject->get('db-folder').'/ '.$params['db-folder'].'/';
            //throw new Exception('You.' . $cmd);
            exec($cmd, $output);
        }

        $oldObject->setAssoc($params);

        $this->database->set($oldObject);
        $this->dispatcher->notify(OMV_NOTIFY_MODIFY, $this->eventMessagePath, $oldObject);

        return $oldObject->getAssoc();
    }

    public function getSettings2($params, $context)
    {
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);

        $object = $this->database->get($this->settings2DataModelPath);

        $port = 8082;

        // Get current port as set in sickbeards config file.
        if (file_exists($object->get('db-folder')."/config.ini")) {
            $config = parse_ini_file(
                $object->get('db-folder')."/config.ini",
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

        $object->add('port', 'integer');
        $object->set('port', intval($port));

        return $object->getAssoc();
    }

    public function setSettings2($params, $context)
    {
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);

        if (empty($params['mntentref']) || $params['mntentref'] == "Select a volume or leave blank for default.") {
            $params['mntentref'] = "Select a volume or leave blank for default.";
            $this->validateMethodParams($params, 'rpc.sickbeard.setsettings2');
            $params['db-folder'] = "/var/opt/sickbeard2";
        } else {
            $this->validateMethodParams($params, 'rpc.sickbeard.setsettings2');
            $db = \OMV\Config\Database::getInstance();
            $deviceDir = $db->getAssoc("conf.system.filesystem.mountpoint", $params['mntentref']);
            $params['db-folder'] = sprintf("{$deviceDir['dir']}/%s", $this->getName());
        }

        // Get existing configuration object
        $oldObject = $this->database->get($this->settings2DataModelPath);

        // Run checks to verify sellected settings
        if ($params['ssl'] == 1) {
            if ($params['ppass'] == 1) {
                throw new Exception('You can not enable both SSL and Proxy Pass.');
            }
            $db = \OMV\Config\Database::getInstance();
            $getInfo = $db->getAssoc("conf.webadmin", $params['enablessl']);
            $xSsl = $getInfo['enablessl'];
            if ($xSsl == "1") {
                throw new Exception('You have SSL enabled for the main UI, Do NOT enable it here as well.');
            }

            $getInfo = $db->getAssoc('conf.system.certificate.ssl');

            if (! array_key_exists('uuid', $getInfo[0])) {
                throw new Exception('You have not generated any SSL certificates.');
            }
        }

        if ($params['showtab'] == 1 && $params['ppass'] != 1) {
            if (! file_exists('/var/www/openmediavault/js/omv/module/admin/service/media/Media.js')) {
                throw new Exception('Install links and add an entry or enable Proxy Pass to use the web tab.');
            }
        }

        if ($params['db-folder'] != $oldObject->get('db-folder') && $oldObject->get('db-folder') != '') {
            $cmd = 'mv '.$oldObject->get('db-folder').'/ '.$params['db-folder'].'/';
            //throw new Exception('You.' . $cmd);
            exec($cmd, $output);
        }

        $oldObject->setAssoc($params);

        $this->database->set($oldObject);
        $this->dispatcher->notify(OMV_NOTIFY_MODIFY, $this->eventMessagePath, $oldObject);

        return $oldObject->getAssoc();
    }


    public function getForks($params, $context)
    {
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);

        // Validate the parameters of the RPC service method.
        //$this->validateMethodParams($params, "rpc.common.getlist");

        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();

	$userRepos = $db->getAssoc($this->forkDataModelPath);

        return $this->applyFilter($userRepos, $params['start'], $params['limit'],
            $params['sortfield'], $params['sortdir']
        );
    }

    public function getFork($params, $context)
    {
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);

        // Validate the parameters of the RPC service method.
        // $this->validateMethodParams($params, "rpc.common.objectuuid");

        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();

        return $db->getAssoc($this->forkDataModelPath, $params['uuid']);
    }

    public function setFork($params, $context)
    {
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, 'rpc.sickbeard.setfork');

        // Check if valid fork. A valid fork should start with either http(s):// or git://
        if (substr($params["fork"], 0, 4) !== "http" &&
            substr($params["fork"], 0, 3) !== "git"
        ) {
            throw new Exception('Not valid git repo.');
        }

        // Query repo for a list of branches.
        $cmd = "git ls-remote -h "
             . $params["fork"]
             . " | sed 's?.*refs/heads/??'";

        $this->exec($cmd, $output);

        if (empty($output)) {
            throw new Exception('No branches found, check url.');
        }

        if (substr($output, 0, 5) === "fatal") {
            throw new Exception('No branches found, is it a valid git repo?');
        }

        if (count($output) == "1") {
            $output[1] = "";
        }

        $object = new \OMV\Config\ConfigObject($this->forkDataModelPath);

        $params['branches'] = $output;

        $object->setAssoc($params);

        $this->database->set($object);

        return $object->getAssoc();
    }

    public function deleteFork($params, $context)
    {
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);

        // Delete the configuration object.
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get($this->forkDataModelPath, $params['uuid']);
        $db->delete($object);

        // Return the deleted configuration object.
        return $object->getAssoc();
    }

    public function enumerateRepos($params, $context)
    {
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);

        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();

        $userRepos = $db->getAssoc($this->forkDataModelPath);

        if (is_null($userRepos)) {
            throw new Exception('No directory for downloads set, please set one before enabling the service');
        }

        // Add default repos and their branches. Do it here so users can't delete them.
        $defaultRepos = [
            [
                "uuid" => "11111111-2222-3333-4444-555555555555",
                "name" => "Main Branch1",
                "fork" => "https://github.com/coach0742/Sick-Beard.git",
                "branches" => [
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
                ]
            ],
            [
                "uuid" => "21111111-2222-3333-4444-555555555555",
                "name" => "Main Branch2",
                "fork" => "https://github.com/SickRage/SickRage.git",
                "branches" => [
                    "master",
                    "develop"
                ]
            ],
            [
                "uuid" => "31111111-2222-3333-4444-555555555555",
                "name" => "Main Branch3",
                "fork" => "https://github.com/cytec/Sick-Beard.git",
                "branches" => [
                    "development",
                    "master",
                    "merges"
                ]
            ],
            [
                "uuid" => "41111111-2222-3333-4444-555555555555",
                "name" => "Main Branch4",
                "fork" => "https://github.com/cytec/SickRage.git",
                "branches" => [
                    "master",
                    "develop",
                    "multilanguage"
                ]
            ]
        ];

        $repos = array_merge($userRepos, $defaultRepos);

        return $repos;
    }

    public function doBackup1($params, $context)
    {
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);

        $db = \OMV\Config\Database::getInstance();

        $deviceDir = $db->getAssoc('conf.system.sharedfolder', $params['sharedfolderref']);
        $deviceDir = $db->getAssoc('conf.system.filesystem.mountpoint', $deviceDir['mntentref'])['dir'] . '/' . $deviceDir['reldirpath'];

        $settings = $db->getAssoc($this->settings1DataModelPath);

        // Sickbeard needs to have run once to generate its settings. Check if this is true.
        if (!file_exists($settings['db-folder'] . "/config.ini")) {
            throw new Exception('No settings folder found!');
        }

        // Create a background process.
        $bgStatusFilename = $this->createBgProcStatus();
        $pid = $this->fork();

        // Parent process.
        if ($pid > 0) {
            $this->initializeBgProcStatus($bgStatusFilename, $pid);
            return $bgStatusFilename;
        }

        // Get the Sickbeard module
        //$moduleMgr = &OMVModuleMgr::getInstance();
        //$sickbeardModule = $moduleMgr->getModule($this->getName());

        // Stop service before restore.
        if (file_exists($settings['db-folder'] . "/config.ini")) {
            $cmd = "service sickbeard stop";
            $this->exec($cmd);
        }

        // Child process.
        try {
            //$sickbeardModule->stopService();
            $bgOutputFilename = $this->createBgProcOutput();
            $this->updateBgProcStatus(
                $bgStatusFilename,
                "outputfilename",
                $bgOutputFilename
            );

            $backupPath = sprintf(
                "%s%ssickbeard.tar.gz",
                $deviceDir,
                DIRECTORY_SEPARATOR
            );

            $cmd = sprintf(
                "tar -zcvPf %s %s 2>&1",
                $backupPath,
                $settings['db-folder']."/"
            );

            $this->exec($cmd, $output, $bgOutputFilename);
            $this->finalizeBgProcStatus($bgStatusFilename, $output);
            //$sickbeardModule->startService();
            // Restart if enabled.
            if ($settings['enable'] = "1") {
                $cmd = "service sickbeard start";
                $this->exec($cmd);
            }
            exit(0);
        } catch (Exception $e) {
            //$sickbeardModule->startService();
            // Restart if enabled.
            if ($settings['enable'] = "1") {
                $cmd = "service sickbeard start";
                $this->exec($cmd);
            }
            $this->finalizeBgProcStatus($bgStatusFilename, "", $e);
            exit(1);
        }
    }

    public function doBackup2($params, $context)
    {
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);

        $db = \OMV\Config\Database::getInstance();

        $deviceDir = $db->getAssoc('conf.system.sharedfolder', $params['sharedfolderref']);
        $deviceDir = $db->getAssoc('conf.system.filesystem.mountpoint', $deviceDir['mntentref'])['dir'] . '/' . $deviceDir['reldirpath'];

        $settings = $db->getAssoc($this->settings2DataModelPath);

        // Sickbeard needs to have run once to generate its settings. Check if this is true.
        if (!file_exists($settings['db-folder'] . "/config.ini")) {
            throw new Exception('No settings folder found!');
        }

        // Create a background process.
        $bgStatusFilename = $this->createBgProcStatus();
        $pid = $this->fork();

        // Parent process.
        if ($pid > 0) {
            $this->initializeBgProcStatus($bgStatusFilename, $pid);
            return $bgStatusFilename;
        }

        // Stop service before backup.
        if (file_exists($settings['db-folder'] . "/config.ini")) {
            $cmd = "service sickbeard2 stop";
            exec($cmd, $output);
        }

        // Child process.
        try {
            $bgOutputFilename = $this->createBgProcOutput();
            $this->updateBgProcStatus(
                $bgStatusFilename,
                "outputfilename",
                $bgOutputFilename
            );

            $backupPath = sprintf(
                "%s%ssickbeard2.tar.gz",
                $deviceDir,
                DIRECTORY_SEPARATOR
            );

            $cmd = sprintf(
                "tar -zcvPf %s %s 2>&1",
                $backupPath,
                $settings['db-folder']."/"
            );

            $this->exec($cmd, $output, $bgOutputFilename);
            $this->finalizeBgProcStatus($bgStatusFilename, $output);
            // Restart if enabled.
            if ($settings['enable'] = "1") {
                $cmd = "service sickbeard2 start";
                exec($cmd, $output);
            }
            exit(0);
        } catch (Exception $e) {
            $this->finalizeBgProcStatus($bgStatusFilename, "", $e);
            // Restart if enabled.
            if ($settings['enable'] = "1") {
                $cmd = "service sickbeard2 start";
                exec($cmd, $output);
            }
            exit(1);
        }
    }

    public function doRestore1($params, $context)
    {
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);

        $db = \OMV\Config\Database::getInstance();

        $deviceDir = $db->getAssoc('conf.system.sharedfolder', $params['sharedfolderref']);
        $deviceDir = $db->getAssoc('conf.system.filesystem.mountpoint', $deviceDir['mntentref'])['dir'] . '/' . $deviceDir['reldirpath'];

        $settings = $db->getAssoc($this->settings1DataModelPath);

        $backupPath = sprintf(
            "%s%ssickbeard.tar.gz",
            $deviceDir,
            DIRECTORY_SEPARATOR
        );

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
            rmdir($settings['db-folder']);

            // Get the Sickbeard module
            //$moduleMgr = &OMVModuleMgr::getInstance();
            //$sickbeardModule = $moduleMgr->getModule($this->getName());

            // Stop service before restore.
            if (file_exists($settings['db-folder'] . "/config.ini")) {
                $cmd = "service sickbeard stop";
                $this->exec($cmd);
            }

            // Child process.
            try {
                //$sickbeardModule->stopService();
                $bgOutputFilename = $this->createBgProcOutput();
                $this->updateBgProcStatus(
                    $bgStatusFilename,
                    "outputfilename",
                    $bgOutputFilename
                );

                $cmd = sprintf("tar -zxvf %s -C / 2>&1", $backupPath);

                $this->exec($cmd, $output, $bgOutputFilename);
                $this->finalizeBgProcStatus($bgStatusFilename, $output);
                //$sickbeardModule->startService();
                // Restart if enabled.
                if ($settings['enable'] = "1") {
                    $cmd = "service sickbeard start";
                    $this->exec($cmd);
                }
                exit(0);
            } catch (Exception $e) {
                $this->finalizeBgProcStatus($bgStatusFilename, "", $e);
                $this->exec("omv-mkconf sickbeard");
                exit(1);
            }
        } else {
            throw new Exception('No backup found!');
        }
    }

    public function doRestore2($params, $context)
    {
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);

        $db = \OMV\Config\Database::getInstance();

        $deviceDir = $db->getAssoc('conf.system.sharedfolder', $params['sharedfolderref']);
        $deviceDir = $db->getAssoc('conf.system.filesystem.mountpoint', $deviceDir['mntentref'])['dir'] . '/' . $deviceDir['reldirpath'];

        $settings = $db->getAssoc($this->settings2DataModelPath);


        $backupPath = sprintf(
            "%s%ssickbeard2.tar.gz",
            $deviceDir,
            DIRECTORY_SEPARATOR
        );

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
            rmdir($settings['db-folder']);

            // Get the Sickbeard module
            //$moduleMgr = &OMVModuleMgr::getInstance();
            //$sickbeardModule = $moduleMgr->getModule($this->getName());

            // Stop service before restore.
            if (file_exists($settings['db-folder'] . "/config.ini")) {
                $cmd = "service sickbeard2 stop";
                $this->exec($cmd);
            }

            // Child process.
            try {
                //$sickbeardModule->stopService();
                $bgOutputFilename = $this->createBgProcOutput();
                $this->updateBgProcStatus(
                    $bgStatusFilename,
                    "outputfilename",
                    $bgOutputFilename
                );

                $cmd = sprintf("tar -zxvf %s -C / 2>&1", $backupPath);

                $this->exec($cmd, $output, $bgOutputFilename);
                $this->finalizeBgProcStatus($bgStatusFilename, $output);
                //$sickbeardModule->startService();
                // Restart if enabled.
                if ($settings['enable'] = "1") {
                    $cmd = "service sickbeard2 start";
                    $this->exec($cmd);
                }
                exit(0);
            } catch (Exception $e) {
                $this->finalizeBgProcStatus($bgStatusFilename, "", $e);
                $this->exec("omv-mkconf sickbeard");
                exit(1);
            }
        } else {
            throw new Exception('No backup found!');
        }
    }

    public function doDelete($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);

        // Create a background process.
        $bgStatusFilename = $this->createBgProcStatus();
        $pid = $this->fork();
        if ($pid > 0)   // Parent process.
        {
            $this->initializeBgProcStatus($bgStatusFilename, $pid);
            return $bgStatusFilename;
        }

        // Child process.
        try {
            $bgOutputFilename = $this->createBgProcOutput();
            $this->updateBgProcStatus($bgStatusFilename, "outputfilename", $bgOutputFilename);

            $cmd = $params['command'];
            $this->exec($cmd, $output, $bgOutputFilename);
            $this->finalizeBgProcStatus($bgStatusFilename, $output);

        } catch(Exception $e) {
            $this->finalizeBgProcStatus($bgStatusFilename, "", $e);
            exit(1);
        }
    }
}
