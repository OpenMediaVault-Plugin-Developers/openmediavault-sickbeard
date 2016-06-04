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

namespace OMV\Engined\Module;

use OMV\Config\Database;
use OMV\Engine\Module\IServiceStatus;
use OMV\Engine\Module\Manager as ModuleManager;
use OMV\Engine\Module\ServiceAbstract;
use OMV\Engine\Notify\Dispatcher;
use OMV\Engine\Notify\IListener;
use OMV\System\Process;
use OMV\System\SystemCtl;

class Sickbeard extends ServiceAbstract implements
    IListener,
    IServiceStatus
{

    private $database;

    private $dataModelPath = 'conf.service.sickbeard';

    private $eventMessagePath = 'org.openmediavault.conf.services.sickbeard';

    private $moduleManager;

    private $serviceName = 'sickbeard.service';

    private $service2Name = 'sickbeard2.service';

    public function __construct(Database $database = null)
    {
        $this->database = $database ?: Database::getInstance();
    }

    public function getName()
    {
        return 'sickbeard';
    }

    public function getStatus()
    {
        $object = $this->database->get($this->dataModelPath);
        $systemCtl = new SystemCtl($this->serviceName);

        return [
            'name' => $this->getName(),
            'title' => gettext('SickBeard'),
            'enabled' => $object->get('enable'),
            'running' => $systemCtl->isActive(),
        ];
    }

    public function applyConfig()
    {
        $cmd = new Process('omv-mkconf', $this->getName());
        $cmd->setRedirect2to1();
        $cmd->execute();
    }

    public function startService()
    {
        $object = $this->database->get($this->dataModelPath);

        if ($object->get('enable')) {

            if ($object->get('branch') == "Please choose a branch") {
                throw new OMVException(
                    OMVErrorMsg::E_MISC_FAILURE,
                    "Please select a repository and branch"
                );
                return;
            }

            if ($object->get('repo') == "Please choose a repository") {
                throw new OMVException(
                    OMVErrorMsg::E_MISC_FAILURE,
                    "Please select a repository and branch"
                );
                return;
            }

            $systemCtl = new SystemCtl($this->serviceName);
            $systemCtl->enable(true);
        }

        if ($object->get('enable')) {

            if ($object->get('branch2') == "Please choose a branch") {
                throw new OMVException(
                    OMVErrorMsg::E_MISC_FAILURE,
                    "Please select a 2nd repository and branch"
                );
                return;
            }

            if ($object->get('repo2') == "Please choose a repository" ) {
                throw new OMVException(
                    OMVErrorMsg::E_MISC_FAILURE,
                    "Please select a 2nd repository and branch"
                );
                return;
            }

            $systemCtl = new SystemCtl($this->service2Name);
            $systemCtl->enable(true);
        }
    }

    public function stopService()
    {
        $systemCtl = new SystemCtl($this->serviceName);
        $systemCtl->disable(true);
    }

    public function bindListeners(Dispatcher $dispatcher)
    {
        $moduleManager = ModuleManager::getInstance();

        $dispatcher->addListener(
            OMV_NOTIFY_MODIFY,
            $this->eventMessagePath,
            [$this, 'setDirty']
        );
    }
}
