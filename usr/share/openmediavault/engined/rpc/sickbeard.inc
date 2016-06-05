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

    private $forksDataModelPath = 'conf.service.sickbeard.forks';

    private $instance1DataModelPath = 'conf.service.sickbeard.instance1';

    private $instance2DataModelPath = 'conf.service.sickbeard.instance2';


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
        //$this->registerMethod("setSettings1");
        $this->registerMethod("getSettings2");
        //$this->registerMethod("setSettings2");

        $this->registerMethod("enumerateRepos");

        $this->registerMethod("getForks");
        $this->registerMethod("getFork");
        //$this->registerMethod("setFork");
        //$this->registerMethod("deleteFork");

        //$this->registerMethod("doBackup1");
        //$this->registerMethod("doRestore1");
        //$this->registerMethod("doBackup2");
        //$this->registerMethod("doRestore2");
        //$this->registerMethod("doDelete");
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

        $object = $this->database->get($this->instance1DataModelPath);

        return $object->getAssoc();
    }

    public function getSettings2($params, $context)
    {
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);

        $object = $this->database->get($this->instance2DataModelPath);

        return $object->getAssoc();
    }


    public function getForks($params, $context)
    {
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);

        $objects = $this->database->get($this->forksDataModelPath);
        //$forks = [];

        //foreach ($objects as $object) {
        //    $forks[] = $object->getAssoc();
        //}

        $test = $this->applyFilter(
            $objects,
            $params['start'],
            $params['limit'],
            $params['sortfield'],
            $params['sortdir']
        );
        file_put_contents('/filename.txt', print_r($test, true));
        return $this->applyFilter(
            $objects,
            $params['start'],
            $params['limit'],
            $params['sortfield'],
            $params['sortdir']
        );
    }

    public function getFork($params, $context)
    {
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);

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
        $xpath = sprintf("%s[uuid='%s']", forksDataModelPath, $params["uuid"]);
        $object = $this->database->get($this->xpath);
        //$object = $xmlConfig->get($xpath);

        if (is_null($object)) {
            throw new Exception('No directory for downloads set, please set one before enabling the service');
        }

        return $object;
    }






    public function enumerateRepos($params, $context)
    {
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);

        
        // Get configuration data.
        $objects = $this->database->get($this->forksDataModelPath);
        $userRepos = $objects->getList("forks");
        //$userRepos = $objects->get("forks");

        if (is_null($userRepos)) {
            throw new Exception('No directory for downloads set, please set one before enabling the service');
        }

        // Add default repos and their branches. Do it here so users can't
        // delete them.
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
}
