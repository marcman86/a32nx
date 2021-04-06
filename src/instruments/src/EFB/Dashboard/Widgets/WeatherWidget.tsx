/* eslint-disable camelcase */

import React, { useEffect, useState } from 'react';
import metarParser from 'aewx-metar-parser';
import { Metar } from '@flybywiresim/api-client';
import { IconWind, IconGauge, IconDroplet, IconTemperature, IconPoint, IconCloud } from '@tabler/icons';

declare type MetarParserType = {
    raw_text: string,
    raw_parts: [string],
    icao: string,
    observed: Date,
    wind: Wind,
    visibility: Visibility,
    conditions: [ConditionCode],
    clouds: [Cloud],
    ceiling: Ceiling,
    temperature: Temperature,
    dewpoint: Dewpoint,
    humidity_percent: number,
    barometer: Barometer,
    flight_category: string,
}

type Wind = {
    degrees: number,
    speed_kts: number,
    speed_mps: number,
    gust_kts: number,
    gust_mps: number,
};

type Visibility = {
    miles: string,
    miles_float: number,
    meters: string,
    meters_float: number,
};

type ConditionCode = {
    code: string,
};

type Cloud = {
    code: string,
    base_feet_agl: number,
    base_meters_agl: number,
};

type Ceiling = {
    code: string,
    feet_agl: number,
    meters_agl: number,
};

type Temperature = {
    celsius: number,
    fahrenheit: number,
};

type Dewpoint = {
    celsius: number,
    fahrenheit: number,
};

type Barometer = {
    hg: number,
    kpa: number,
    mb: number,
};

const MetarParserTypeWindState: Wind = {
    degrees: 0,
    speed_kts: 0,
    speed_mps: 0,
    gust_kts: 0,
    gust_mps: 0,
};

const VisibilityType = {
    miles: '',
    miles_float: 0.0,
    meters: '',
    meters_float: 0.0,
};

const conditionCode = { code: '' };

const cloud = {
    code: '',
    base_feet_agl: 0,
    base_meters_agl: 0,
};

const ceiling = {
    code: '',
    feet_agl: 0,
    meters_agl: 0,
};

const temperature = {
    celsius: 0,
    fahrenheit: 0,
};

const dewpoint = {
    celsius: 0,
    fahrenheit: 0,
};

const barometer = {
    hg: 0,
    kpa: 0,
    mb: 0,
};

const MetarParserTypeProp: MetarParserType = {
    raw_text: '',
    raw_parts: [''],
    icao: '',
    observed: new Date(0),
    wind: MetarParserTypeWindState,
    visibility: VisibilityType,
    conditions: [conditionCode],
    clouds: [cloud],
    ceiling,
    temperature,
    dewpoint,
    humidity_percent: 0,
    barometer,
    flight_category: '',
};

type WeatherWidgetProps = { name: string, editIcao: string, icao: string, cardRight: boolean };

const WeatherWidget = (props: WeatherWidgetProps) => {
    const [metar, setMetar] = useState<MetarParserType>(MetarParserTypeProp);

    // This could be modified using the Settings tab perhaps?
    const source = 'vatsim';

    const handleIcao = (event: { target: { value: React.SetStateAction<string>; }; }) => {
        if (event.target.value.length === 4) {
            getMetar(event.target.value, source);
        } else if (event.target.value.length === 0) {
            getMetar(props.icao, source);
        }
    };

    function getMetar(icao:any, source: any) {
        if (icao.length !== 4 || icao === '----') {
            return new Promise(() => {
                setMetar(MetarParserTypeProp);
            });
        }
        return Metar.get(icao, source)
            .then((result) => {
                const metarParse = metarParser(result.metar);
                console.info(metarParse);
                setMetar(metarParse);
            })
            .catch(() => {
                setMetar(MetarParserTypeProp);
            });
    }

    useEffect(() => {
        getMetar(props.icao, source);
    }, [props.icao]);

    return (
        <div className="text-white">
            {metar === undefined
                ? <p>Loading ...</p>
                : (
                    <>
                        <div className="mb-6">
                            {props.cardRight ? (
                                <div className="inline-flex items-center w-64">
                                    <input
                                        className="text-right mr-4 border-none focus:outline-none text-2xl bg-transparent font-medium uppercase"
                                        type="text"
                                        placeholder={props.icao}
                                        onChange={handleIcao}
                                    />
                                    <div className="mr-8">
                                        <IconCloud size={35} stroke={1.5} strokeLinejoin="miter" />
                                    </div>
                                </div>
                            ) : (
                                <div className="inline-flex items-center w-64 overflow-hidden">
                                    <div className="ml-8">
                                        <IconCloud size={35} stroke={1.5} strokeLinejoin="miter" />
                                    </div>
                                    <input
                                        className="text-left ml-4 border-none focus:outline-none text-2xl bg-transparent font-medium uppercase"
                                        type="text"
                                        placeholder={props.icao}
                                        onChange={handleIcao}
                                    />
                                </div>
                            )}
                        </div>
                        <div className="grid grid-cols-2">
                            <div className="text-center text-lg">
                                <div className="flex justify-center">
                                    <IconGauge className="mb-2" size={35} stroke={1.5} strokeLinejoin="miter" />
                                </div>
                                {metar.barometer ? (
                                    <>
                                        {metar.barometer.mb.toFixed(0)}
                                        {' '}
                                        mb
                                    </>
                                ) : 'N/A'}
                            </div>
                            <div className="text-center text-lg">
                                <div className="flex justify-center">
                                    <IconWind className="mb-2" size={35} stroke={1.5} strokeLinejoin="miter" />
                                </div>
                                {metar.wind
                                    ? (
                                        <>
                                            {metar.wind.degrees.toFixed(0)}
                                            {' '}
                                            <IconPoint
                                                className="inline-block -mx-1 -mt-3"
                                                size={20}
                                                stroke={2}
                                                strokeLinejoin="miter"
                                            />
                                            {' '}
                                            /
                                            {' '}
                                            {metar.wind.speed_kts.toFixed(0)}
                                            {' '}
                                            kts
                                        </>
                                    ) : 'N/A'}
                            </div>
                            <div className="text-center text-lg mt-6">
                                <div className="flex justify-center">
                                    <IconTemperature className="mb-2" size={35} stroke={1.5} strokeLinejoin="miter" />
                                </div>
                                {metar.temperature
                                    ? (
                                        <>
                                            {metar.temperature.celsius.toFixed(0)}
                                            {' '}
                                            <IconPoint
                                                className="inline-block -mx-1 -mt-3"
                                                size={20}
                                                stroke={2}
                                                strokeLinejoin="miter"
                                            />
                                            {' '}
                                            {' '}
                                            C
                                        </>
                                    ) : 'N/A'}
                            </div>
                            <div className="text-center text-lg mt-6">
                                <div className="flex justify-center">
                                    <IconDroplet className="mb-2" size={35} stroke={1.5} strokeLinejoin="miter" />
                                </div>
                                {metar.dewpoint
                                    ? (
                                        <>
                                            {metar.dewpoint.celsius.toFixed(0)}
                                            {' '}
                                            <IconPoint
                                                className="inline-block -mx-1 -mt-3"
                                                size={20}
                                                stroke={2}
                                                strokeLinejoin="miter"
                                            />
                                            {' '}
                                            {' '}
                                            C
                                        </>
                                    ) : 'N/A'}
                            </div>
                        </div>
                    </>
                )}
        </div>
    );
};

export default WeatherWidget;
